#ifdef _WIN32 // Windows includes
#pragma comment(lib, "ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS // Disable the "deprecated" warning for inet_addr()
// I tried using inet_pton() like the warning suggested, but it didnt work and I dont really care to fix it
#include <winsock2.h>
#include <ws2tcpip.h>
#include <strsafe.h>
#include <windows.h>
#else // Linux stuff
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <err.h>
#include <cstring>
#endif // Any includes below here are cross-platform
#include <stdio.h>
#include <string>


namespace torPlusPlus{

bool DEBUG = true; // Whether or not to print debug messages
bool DEBUG_V = true; // Whether or not to print potentially LOTS of debug messages (send/recv)
#define DEBUG_printf(...) if(DEBUG) printf(__VA_ARGS__) // Macro that, if DEBUG is true, will printf debug message
#define DEBUG_V_printf(...) if(DEBUG_V) printf(__VA_ARGS__) // Macro that, if DEBUG_V is true, will printf debug message

// Some define stuff for Linux:
#ifndef _WIN32
#define closesocket close
#define SOCKET_ERROR    -1
#define INVALID_SOCKET  -1
#define WSACleanup() // Define cleanup to nothing on Linux
#define WSAGetLastError() errno
#define SOCKADDR struct sockaddr
typedef int SOCKET;
#endif

// Use inet_pton() to check if a given const char* is an IPv6 address
bool isIPv6(const char* ip){
    struct sockaddr_in sa;
    return inet_pton(AF_INET6, ip, &(sa.sin_addr)) != 0;
}

// Returns a string containing the error message for a given SOCKS5 error code
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

// The base torSocket class
class torSocket{
protected:
#ifdef _WIN32
    PROCESS_INFORMATION torProxyProcess = {0}; // This is stored so we can terminate the proxy process when the class is destroyed
    WSADATA wsaData = {0}; // Storing this isnt really necessary. Holds info about the Winsock implementation
#endif
    SOCKET torProxySocket = {0}; // The socket used to connect to the proxy
    sockaddr_in torProxyAddr = {0}; // The address of the proxy (usually 127.0.0.1:9050)
    bool connected = false; // Whether or not we have successfully connected to the proxy and authenticated yet
    bool socks5ConnectedToHost = false; // Whether or not we have successfully connected to the host through the proxy
    
public:
    /*
        Closes the socket, cleans up Winsock, and terminates the proxy process
    */
    ~torSocket(){
        closesocket(this->torProxySocket);
        WSACleanup(); // Does some "cleaup" stuff on Windows
#ifdef _WIN32
        TerminateProcess(this->torProxyProcess.hProcess, 0); // Terminates the proxy process
#endif
    }

    /*
        startTorProxy()
        Starts the Tor proxy executable as a separate process
        Arguments:
            torPath: The path to the tor.exe executable. Defaults to ".\\tor\\tor.exe"
        Returns:
            1 if the proxy process was started successfully
            0 if the proxy process failed to start
    */
#ifdef _WIN32
    int startTorProxy(const char* torPath = ".\\tor\\tor.exe"){
        wchar_t* lpTorPath = new wchar_t[strlen(torPath) + 1]; // Create a wchar_t* to store the path to the tor.exe executable
        mbstowcs_s(NULL, lpTorPath, strlen(torPath) + 1, torPath, strlen(torPath) + 1); // Converts the path to the tor.exe executable to a wchar_t*
        STARTUPINFOW startupInfo;
        ZeroMemory(&startupInfo, sizeof(startupInfo));
        startupInfo.cb = sizeof(startupInfo);
        ZeroMemory(&this->torProxyProcess, sizeof(this->torProxyProcess));
        BOOL success = CreateProcessW(lpTorPath, 
                                      NULL, 
                                      NULL, 
                                      NULL, 
                                      FALSE, 
                                      0, 
                                      NULL, 
                                      NULL, 
                                      &startupInfo, 
                                      &this->torProxyProcess
        );
        delete[] lpTorPath; // Free the memory allocated for the path
        if(!success){ // If CreateProcessW() failed
            DEBUG_printf("startTorProxy(): ERR: CreateProcessW failed: %d\n", GetLastError());
            return 0; // Return 0 to indicate failure
        }
        DEBUG_printf("startTorProxy(): Tor proxy executable started\n");
        return 1;
    }
#else
    int startTorProxy(const char* torPath = "."){
        DEBUG_printf("startTorProxy(): ERR: startTorProxy() is not implemented on Linux, if TOR is already running then you can ignore this error\n");
        return 1; // Returning success, as we hope TOR is already running!
    }
#endif

    /*
        connectToProxy()
        Connects to the Tor proxy
        Arguments:
            torProxyIP: The IP address of the proxy (almost always 127.0.0.1)
            torProxyPort: The port of the proxy (almost always 9050)
        Returns:
            1 on success
            0 on error
    */
    int connectToProxy(const char* torProxyIP = "127.0.0.1",
                       const int torProxyPort = 9050
                       ){
#ifdef _WIN32
        // Initialize Winsock
        int WSAStartupResult = WSAStartup(MAKEWORD(2, 2), &this->wsaData); // MAKEWORD(2,2) specifies version 2.2 of Winsock
        if(WSAStartupResult != 0){ // WSAStartup returns 0 on success
            DEBUG_printf("connectToProxy(): ERR: WSAStartup failed with error: %d\n", WSAStartupResult);
            return 0;
        }
#endif
        // === Create a SOCKET for connecting to the proxy
        this->torProxySocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if(this->torProxySocket == INVALID_SOCKET){ // If the socket failed to create, abort the connection attempt
            DEBUG_printf("connectToProxy(): ERR: socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 0;
        }
        // === Setup the proxy address structure:
        this->torProxyAddr.sin_family = AF_INET; // IPv4
        this->torProxyAddr.sin_addr.s_addr = inet_addr(torProxyIP);
        this->torProxyAddr.sin_port = htons(torProxyPort);
        // === Connect to the proxy:
        int connectResult = connect(this->torProxySocket, (SOCKADDR*)&this->torProxyAddr, sizeof(this->torProxyAddr));
        if(connectResult == SOCKET_ERROR){ // If the connection failed, abort the connection attempt
            DEBUG_printf("connectToProxy(): ERR: connect failed with error: %ld\n", WSAGetLastError());
            closesocket(this->torProxySocket);
            WSACleanup();
            return 0;
        }
        DEBUG_printf("connectToProxy(): Connected to proxy at %s:%d\n", torProxyIP, torProxyPort);
        // === Authenticate with the proxy:
        char authReq[3] = {0x05, 0x01, 0x00}; // SOCKS5, 1 auth method, no auth
        send(this->torProxySocket, authReq, sizeof(authReq), 0);
        char authResp[2]; // 2 chars is enough for the response, the second char is the error code.
        recv(this->torProxySocket, authResp, sizeof(authResp), 0);
        if(authResp[1] != 0x00){ // 0x00 indicates success, anything else is an error
            DEBUG_printf("connectToProxy(): ERR: Proxy authentication failed with error: %d\n", authResp[1]);
            closesocket(this->torProxySocket);
            WSACleanup();
            return 0;
        }
        DEBUG_printf("connectToProxy(): Proxy authentication successful\n");
        this->connected = true; // We have successfully connected to the proxy and authenticated, so set this->connected to true
        // this->connected is used to ensure that a connection is established before attempting to send data through the proxy
        return 1;
    }

    /*
        startAndConnectToProxy()
        Combines the startTorProxy() and connectToProxy() functions into one function for convenience
        Arguments:
            torPath: The path to the tor.exe executable
            torProxyIP: The IP address of the proxy (almost always 127.0.0.1)
            torProxyPort: The port of the proxy (almost always 9050)
        Returns:
            1 on success, 0 on failure. Uses the value of this->connected to determine success or failure if this->startTorProxy() returns 1
    */
    int startAndConnectToProxy(const char* torPath = ".\\tor\\tor.exe",
                               const char* torProxyIP = "127.0.0.1",
                               const int torProxyPort = 9050
                               ){
        // === Start the Tor proxy:
        if(!this->startTorProxy(torPath)){ // If the Tor proxy failed to start, abort the connection attempt
            DEBUG_printf("startAndConnectToProxy(): ERR: Tor proxy failed to start\n");
            return 0;
        }
        // === Connect to the proxy:
        this->connectToProxy(torProxyIP, torProxyPort);
        return this->connected; // Return the value of this->connected to indicate success or failure
    }

    /*
        connectProxyTo()
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
            DEBUG_printf("connectProxyTo(): ERR: Not connected to proxy\n");
            return -1;
        }
        if(isIPv6(host)){
            DEBUG_printf("connectProxyTo(): ERR: Host is an IPv6 address, which is not supported by the TOR SOCKS5 proxy\n");
            return -1;
        }
        DEBUG_printf("connectProxyTo(): Attempting to connect to %s:%d\n", host, port);
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
        DEBUG_printf("connectProxyTo(): Sending connect request to proxy\n");
        send(this->torProxySocket, connectReq, 7 + hostLen, 0); // Send the connect request to the proxy // TODO: ERROR CHECKING
        delete[] connectReq; // Free the memory used to store the connect request
        // === Get the connect response:
        DEBUG_printf("connectProxyTo(): Waiting for connect response from proxy...\n");
        char connectResp[10]; // A buffer to hold the connect response from the proxy
        recv(this->torProxySocket, connectResp, sizeof(connectResp), 0); // Receive the connect response from the proxy
        if(connectResp[1] != 0x00){ // If the proxy responded with an error, abort the connection attempt
            DEBUG_printf("connectProxyTo(): ERR: Proxy connection failed with error: %d: %s\n", connectResp[1], getSocks5Error(connectResp[1]));
            return (int)connectResp[1]; // Return SOCKS5 err code
        }
        DEBUG_printf("connectProxyTo(): Successfully connected to %s:%d\n", host, port);
        this->socks5ConnectedToHost = true; // We have successfully connected to the host through the proxy, so set this->socks5ConnectedToHost to true
        return 0;
    }

    /*
        proxySend()
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
            DEBUG_printf("proxySend(): ERR: Not connected to proxy/remote host\n");
            return -1;
        }
        DEBUG_V_printf("proxySend(): Sending %d bytes...\n", len);
        int sent = send(this->torProxySocket, data, len, 0);
        DEBUG_V_printf("proxySend(): Sent %d bytes\n", sent);
        return sent;
    }
    
    /*
        proxyRecv()
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
            DEBUG_printf("proxyRecv(): ERR: Not connected to proxy\n");
            return -1;
        }
        DEBUG_V_printf("proxyRecv(): Receiving (up to) %d bytes...\n", len);
        int received = recv(this->torProxySocket, data, len, 0);
        DEBUG_V_printf("proxyRecv(): Received %d bytes\n", received);
        return received;
    }

    /*
        closeTorSocket()
        Stops the proxy and closes the socket
    */
    void closeTorSocket(){ // Had to change the name of this function to avoid conflicts with close()
        if(this->connected){ // If we are connected to the proxy, close the socket and terminate the proxy process
            DEBUG_printf("close(): Closing proxy socket\n");
            closesocket(this->torProxySocket);
#ifdef _WIN32
            DEBUG_printf("close(): Running WSACleanup\n");
            WSACleanup();
            DEBUG_printf("close(): Terminating proxy process\n");
            if(TerminateProcess(this->torProxyProcess.hProcess, 0) == 0){
                DEBUG_printf("close(): ERR: Failed to terminate proxy process\n");
            }
#endif
            this->connected = false; // Set this->connected to false
            this->socks5ConnectedToHost = false; // Set this->socks5ConnectedToHost to false
        }else{ // Otherwise just attempt to terminate the proxy process (if on windows)
#ifdef _WIN32
            DEBUG_printf("close(): ERR: Not connected to proxy - attempting to terminate proxy process anyway\n");
            TerminateProcess(this->torProxyProcess.hProcess, 0);
#else
            DEBUG_printf("close(): ERR: Not connected to proxy\n");
#endif
        }
    }

    /*
        getSocket()
        Returns the socket used to connect to the proxy, allowing you to use the socket directly (in case you need to do something that this class doesn't support)
    */
    SOCKET getSocket(){
        return this->torProxySocket;
    }

};

}

namespace torPlusPlus{

/*
    torSocketExtended
    A class that extends torSocket to add some useful functions
*/
class torSocketExtended : public torSocket{
public:
    // !!! Not suitable for binary data
    std::string proxyRecvStrUntilNewln(){ 
        std::string data;
        char buffer[1024];
        int received;
        while((received = this->proxyRecv(buffer, sizeof(buffer))) > 0){
            data.append(buffer, received);
            if(data.find("\n") != std::string::npos){
                break;
            }
        }
        return data;
    }

    // !!! Not suitable for binary data
    int proxySendStr(std::string data){
        return this->proxySend(data.c_str(), data.length());
    }
    
};

}
