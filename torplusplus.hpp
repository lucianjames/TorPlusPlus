#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <err.h>
#include <cstring>
#include <fstream>
#include <signal.h>
#include <iostream>
#include <stdarg.h>

/*

    REWRITING ON LINUX ONLY FOR NOW
    WILL ADD WINDOWS SUPPORT LATER

*/

namespace torPlusPlus{


#define closesocket close
#define SOCKET_ERROR    -1
#define INVALID_SOCKET  -1
#define WSACleanup() // Define cleanup to nothing on Linux
#define WSAGetLastError() errno
#define SOCKADDR struct sockaddr
typedef int SOCKET;


// Use inet_pton() to check if a given const char* is an IPv6 address
bool isIPv6(const char* ip){
    struct sockaddr_in sa;
    return inet_pton(AF_INET6, ip, &(sa.sin_addr)) != 0;
}


/*
    Returns a string containing the error message for a given SOCKS5 error code
*/
const char* getSocks5Error(const int error){
    switch(error){
        case 0x00: return "Succeeded";
        case 0x01: return "General SOCKS server failure";
        case 0x02: return "Connection not allowed by ruleset";
        case 0x03: return "Network unreachable";
        case 0x04: return "Host unreachable";
        case 0x05: return "Connection refused";
        case 0x06: return "TTL expired";
        case 0x07: return "Command not supported";
        case 0x08: return "Address type not supported";
        default: return "Unknown error";
    }
}


class TORSocket{
protected:
    SOCKET torProxySocket = {0};
    sockaddr_in torProxyAddr = {0};

    bool debug = false;
    bool connected = false;
    bool socks5ConnectedToHost = false;

    void DEBUG_printf(const char* format, ...){
        if(this->debug){
            va_list args;
            va_start(args, format);
            vprintf(format, args);
            va_end(args);
        }
    }

public:
    /*
        Constructor
        Arguments:
            debug: If true, enables debug output to stdout
    */
    TORSocket(const bool debug = false){
        this->debug = debug;
    }

    /*
        Connects to the Tor proxy
        Arguments:
            torProxyIP: The IP address of the proxy (almost always 127.0.0.1)
            torProxyPort: The port of the proxy (almost always 9050)
        Returns:
            0 on success,
            -1 on failure
    */
    int connectToProxy(const int torProxyPort = 9050,
                       const char* torProxyIP = "127.0.0.1"
                       ){
        // === Create a SOCKET for connecting to the proxy
        this->torProxySocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if(this->torProxySocket == INVALID_SOCKET){ // If the socket failed to create, abort the connection attempt
            this->DEBUG_printf("connectToProxy(): ERR: socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return -1;
        }
        // === Setup the proxy address structure:
        this->torProxyAddr.sin_family = AF_INET; // IPv4
        this->torProxyAddr.sin_addr.s_addr = inet_addr(torProxyIP);
        this->torProxyAddr.sin_port = htons(torProxyPort);
        // === Connect to the proxy:
        int connectResult = connect(this->torProxySocket, (SOCKADDR*)&this->torProxyAddr, sizeof(this->torProxyAddr));
        if(connectResult == SOCKET_ERROR){ // If the connection failed, abort the connection attempt
            this->DEBUG_printf("connectToProxy(): ERR: connect failed with error: %ld\n", WSAGetLastError());
            closesocket(this->torProxySocket);
            WSACleanup();
            return -1;
        }
        this->DEBUG_printf("connectToProxy(): Connected to proxy at %s:%d\n", torProxyIP, torProxyPort);
        // === Authenticate with the proxy:
        char authReq[3] = {0x05, 0x01, 0x00}; // SOCKS5, 1 auth method, no auth
        send(this->torProxySocket, authReq, sizeof(authReq), 0);
        char authResp[2]; // 2 chars is enough for the response, the second char is the error code.
        recv(this->torProxySocket, authResp, sizeof(authResp), 0);
        if(authResp[1] != 0x00){ // 0x00 indicates success, anything else is an error
            this->DEBUG_printf("connectToProxy(): ERR: Proxy authentication failed with error: %d\n", authResp[1]);
            closesocket(this->torProxySocket);
            WSACleanup();
            return -1;
        }
        this->DEBUG_printf("connectToProxy(): Proxy authentication successful\n");
        this->connected = true; // We have successfully connected to the proxy and authenticated, so set this->connected to true
        // this->connected is used to ensure that a connection is established before attempting to send data through the proxy
        return 0;
    }

    /*
        Connects to a host through the proxy.
        Can be passed a domain name or an IP address
        Arguments:
            host: The host to connect to
            port: The port to connect to
        Returns:
            SOCKS5 err code (0 on success, see RFC1928 section 6)
            Returns -1 on other failures
    */
    int connectProxyTo(const char* host, 
                       const int port=80){
        // === Initial checking and setup:
        if(!this->connected){ // If we are not connected to the proxy, abort the connection attempt and give an error message to the stupid developer who forgot to connect to the proxy
            this->DEBUG_printf("connectProxyTo(): ERR: Not connected to proxy\n");
            return -1;
        }
        if(isIPv6(host)){
            this->DEBUG_printf("connectProxyTo(): ERR: Host is an IPv6 address, which is not supported by the TOR SOCKS5 proxy\n");
            return -1;
        }
        this->DEBUG_printf("connectProxyTo(): Attempting to connect to %s:%d\n", host, port);
        // === Assemble a SOCKS5 connect request:
        char hostLen = (char)strlen(host); // Get the length of the domain, as a char so it can be sent as a single byte
        char* connectReq = new char[7 + hostLen]; // 7 bytes for the SOCKS5 header, plus the length of the domain
        connectReq[0] = 0x05; // Specify that we are using the  SOCKS5 protocol
        connectReq[1] = 0x01; // Specify that this is a connect command
        connectReq[2] = 0x00; // Reserved
        connectReq[3] = 0x03; // Specify that the host is a domain name
        connectReq[4] = hostLen; // Domain length
        memcpy(connectReq + 5, host, hostLen); // Copy the domain into the connect request, starting at byte 5
        short portN = htons(port); // Convert the port to network byte order.
        memcpy(connectReq + 5 + hostLen, &portN, 2); // Copy the port into the connect request, starting at byte <domain length> + 5
        // === Send the connect request:
        this->DEBUG_printf("connectProxyTo(): Sending connect request to proxy\n");
        send(this->torProxySocket, connectReq, 7 + hostLen, 0); // Send the connect request to the proxy // TODO: ERROR CHECKING
        delete[] connectReq; // Free the memory used to store the connect request
        // === Get the connect response:
        this->DEBUG_printf("connectProxyTo(): Waiting for connect response from proxy...\n");
        char connectResp[10]; // A buffer to hold the connect response from the proxy
        recv(this->torProxySocket, connectResp, sizeof(connectResp), 0); // Receive the connect response from the proxy
        if(connectResp[1] != 0x00){ // If the proxy responded with an error, abort the connection attempt
            this->DEBUG_printf("connectProxyTo(): ERR: Proxy connection failed with error: %d: %s\n", connectResp[1], getSocks5Error(connectResp[1]));
            return (int)connectResp[1]; // Return SOCKS5 err code
        }
        this->DEBUG_printf("connectProxyTo(): Successfully connected to %s:%d\n", host, port);
        this->socks5ConnectedToHost = true; // We have successfully connected to the host through the proxy, so set this->socks5ConnectedToHost to true
        return 0;
    }

    /*
        Sends data through the proxy to the service we are connected to
        Arguments:
            data: The data to send
            len: The length of the data to send
        Returns:
            The number of bytes sent (or -1 if an error occurred)
    */
    int proxySend(const char* data, 
                  const int len){
        if(!(this->connected && this->socks5ConnectedToHost)){ // Ensure that we are connected to the proxy and to the host through the proxy
            this->DEBUG_printf("proxySend(): ERR: Not connected to proxy/remote host\n");
            return -1;
        }
        //DEBUG_V_printf("proxySend(): Sending %d bytes...\n", len);
        int sent = send(this->torProxySocket, data, len, 0);
        //DEBUG_V_printf("proxySend(): Sent %d bytes\n", sent);
        return sent;
    }
    
    /*
        Overload for proxySend to send std::strings without doing .c_str() and .length() every time
    */
    int proxySend(std::string data){
        return this->proxySend(data.c_str(), data.length());
    }
    
    /*
        Receives data from the proxy
        Arguments:
            data: A pointer to a buffer to store the received data
            len: The maximum number of bytes to receive
        Returns:
            The number of bytes received (or -1 if an error occurred)
    */
    int proxyRecv(char* data, 
                  const int len){
        if(!(this->connected && this->socks5ConnectedToHost)){  // Ensure that we are connected to the proxy and to the host through the proxy
            this->DEBUG_printf("proxyRecv(): ERR: Not connected to proxy\n");
            return -1;
        }
        //DEBUG_V_printf("proxyRecv(): Receiving (up to) %d bytes...\n", len);
        int received = recv(this->torProxySocket, data, len, 0);
        //DEBUG_V_printf("proxyRecv(): Received %d bytes\n", received);
        return received;
    }

};


class TOR{
protected:
    int torPort;
    std::string torrcPath;
    std::string torExePath;
    bool debug;
    bool proxyRunning = false;
    pid_t torProxyProcess = 0;
    sockaddr_in torProxyAddr = {0};

    void DEBUG_printf(const char* format, ...){
        if(this->debug){
            va_list args;
            va_start(args, format);
            vprintf(format, args);
            va_end(args);
        }
    }

    void waitForProxy(){
        // === Wait for TOR to start by attempting a connection to 127.0.0.1:torPort every 250ms until it succeeds
        this->DEBUG_printf("waitForProxy(): Waiting for TOR\n");
        int torProxyTestSocket = socket(AF_INET, SOCK_STREAM, 0); // Not using the SOCKET typedef because this version of this function wont be running on windows
        sockaddr_in torProxyTestAddr = {0};
        torProxyTestAddr.sin_family = AF_INET;
        torProxyTestAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        torProxyTestAddr.sin_port = htons(this->torPort);
        while(connect(torProxyTestSocket, (struct sockaddr*)&torProxyTestAddr, sizeof(this->torProxyAddr)) != 0){
            usleep(250000);
        }
        close(torProxyTestSocket);
        this->DEBUG_printf("waitForProxy(): Done\n");
    }

    /*
        Starts the TOR proxy (via the command line)
        Returns:
            0 on success
            -1 on failure
    */
    int startTorProxy(){
        FILE* torTest = popen("tor --version", "r"); // Check if TOR can be run from the command line
        if(torTest == NULL){
            this->DEBUG_printf("startTorProxy(): ERR: Failed to run TOR from command line\n");
            return -1;
        }
        pclose(torTest);
        this->torProxyProcess = fork();
        if(this->torProxyProcess == 0){ // If inside the child process
            this->DEBUG_printf("startTorProxy(): Starting TOR\n");
            execlp("tor", "tor", "-f", this->torrcPath.c_str(), NULL);
            this->DEBUG_printf("startTorProxy(): ERR: Failed to start TOR\n");
            return -1;
        }
        // === Wait for TOR to start
        this->waitForProxy();
        this->DEBUG_printf("startTorProxy(): TOR started\n");
        return 0;
    }

    /*
        Starts the TOR proxy by running an executable file located at this->torExePath
        Returns:
            0 on success
            -1 on failure
    */
    int startTorProxyFromFile(){
        this->torProxyProcess = fork();
        if(this->torProxyProcess == 0){
            this->DEBUG_printf("startTorProxyFromFile(): Starting TOR\n");
            execlp(this->torExePath.c_str(), this->torExePath.c_str(), "-f", this->torrcPath.c_str(), NULL);
            this->DEBUG_printf("startTorProxyFromFile(): ERR: Failed to start TOR\n");
            return -1;
        }
        // === Wait for TOR to start
        this->waitForProxy();
        this->DEBUG_printf("startTorProxy(): TOR started\n");
        return 0;
    }

    /*
        Clears the torrc file, and re-creates it with the SocksPort set to this->torPort
        Also adds "Log notice file /dev/null" to the torrc file if this->debug is false
        (This reduces the amount of debug info that TOR outputs to the console)
    */
    void createTorrc(){
        // === Remove old torrc file if it exists, to ensure no old settings are used
        remove(this->torrcPath.c_str());
        // === Create a new torrc file and set the SocksPort to the port specified in this->torPort
        std::ofstream torrcFile(this->torrcPath);
        torrcFile << "SocksPort " << this->torPort << std::endl;
        if(!this->debug){
            torrcFile << "Log notice file /dev/null" << std::endl;
        }
        torrcFile.close();
    }

public:
    /*
        Constructor
        Arguments:
            torPort: The port to run the TOR proxy on
            torrcPath: The path to create the TOR configuration file at
            debug: Whether or not to print debug info to the console
    */
    TOR(const int torPort = 9050, const std::string& torrcPath = ".tpptorrc", const bool debug = false, const std::string& torExePath = "tor"){
        this->torPort = torPort;
        this->torrcPath = torrcPath;
        this->debug = debug;
        this->createTorrc();
        this->torExePath = torExePath;
    }

    /*
        Destructor
        Stops TOR and removes the configuration file
    */
    ~TOR(){
        this->stop();
        remove(this->torrcPath.c_str());
    }

    /*
        Stops the TOR proxy
        Returns:
            0 on success
            -1 on failure
    */
    int stop(){
        if(this->proxyRunning){
            this->DEBUG_printf("stop(): Stopping TOR\n");
            kill(this->torProxyProcess, SIGTERM);
            this->proxyRunning = false;
        }
        return 0;
    }

    /*
        Starts the TOR proxy
        Returns:
            0 on success
            -1 on failure
    */
    int start(){
        if(!this->proxyRunning){
            this->proxyRunning = true;
            return this->startTorProxy();
        }else{
            this->DEBUG_printf("start(): ERR: TOR proxy already running\n");
            return -1;
        }
    }

    /*
        Starts the TOR proxy, using an executable file directly. Located at this->torExePath
        Returns:
            0 on success
            -1 on failure
    */
    int startFromFile(){
        if(!this->proxyRunning){
            this->proxyRunning = true;
            return this->startTorProxyFromFile();
        }else{
            this->DEBUG_printf("startFromFile(): ERR: TOR proxy already running\n");
            return -1;
        }
    }

    /*
        Restarts the TOR proxy
        Returns:
            0 on success
            -1 on failure
    */
    int restart(){
        this->stop();
        return this->start();
    }
    
    /*
        Adds a service to the TOR configuration, allowing you to run servers using this TOR instance
        Requires a restart of the TOR proxy to take effect
        Arguments:
            servicePath: The path to the folder containing the service's keys
            serviceTORPort: The port to run the service on
            servicePort: The port to redirect requests to the service to
        Returns:
            0 on success
            -1 on failure
    */
    int addService(const std::string& servicePath, const int serviceTORPort, const int servicePort){
        if(this->proxyRunning){
            this->DEBUG_printf("addService(): WARN: Service will not be accessible until TOR is restarted\n");
        }
        std::ofstream torrcFile(this->torrcPath, std::ios_base::app);
        torrcFile << "HiddenServiceDir " << servicePath << std::endl;
        // HiddenServicePort x y:z says to redirect requests on port x to the address y:z.
        torrcFile << "HiddenServicePort " << serviceTORPort << "127.0.0.1:" << servicePort << std::endl;
        torrcFile.close();
        return 0;
    }

    /*
        Creates and connects a socket to this TOR instance
    */
    TORSocket getSocket(){
        TORSocket ts(this->debug);
        ts.connectToProxy(this->torPort);
        return ts;
    }

};


}
