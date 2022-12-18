#pragma comment(lib, "ws2_32.lib")

#define _WINSOCK_DEPRECATED_NO_WARNINGS // Disable the "deprecated" warning for inet_addr()
// I tried using inet_pton() like the warning suggested, but it didnt work and I dont really care to fix it

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <strsafe.h>

namespace torPlusPlus{

#define DEBUG true // Set to true to enable debug messages - Using macro instead of variable so that strings arent stored in the binary if debugging is turned off
#define DEBUG_printf(...) if(DEBUG) printf(__VA_ARGS__) // Macro that, if DEBUG is true, will printf debug message

// Use inet_addr() to check if a given const char* is an IPv4 address
bool isIPv4(const char* ip){
    return inet_addr(ip) != INADDR_NONE;
}

// Use inet_pton() to check if a given const char* is an IPv6 address
bool isIPv6(const char* ip){
    struct sockaddr_in sa;
    return inet_pton(AF_INET6, ip, &(sa.sin_addr)) != 0;
}

// Returns a string containing the error message for a given SOCKS5 error code
#if DEBUG
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
#else
const char* getSocks5Error(const int error){
    return ""; // If debugging is turned off, then we dont want to store strings for the error messages in the binary. Totally not to make it harder to reverse engineer the binary
}
#endif

// The torSocket class
class torSocket{
protected:
    // === Declare some private variables:
    PROCESS_INFORMATION torProxyProcess = {0}; // This is stored so we can terminate the proxy process when the class is destroyed
    WSADATA wsaData = {0}; // Storing this isnt really necessary. Holds info about the Winsock implementation
    SOCKET torProxySocket = {0}; // The socket used to connect to the proxy
    sockaddr_in torProxyAddr = {0}; // The address of the proxy (usually 127.0.0.1:9050)
    bool connected = false; // Whether or not we have successfully connected to the proxy and authenticated yet
    bool socks5ConnectedToHost = false; // Whether or not we have successfully connected to the host through the proxy
    
    /*
        startTorProxy()
        Starts the Tor proxy executable as a separate process
        Arguments:
            torPath: The path to the tor.exe executable
    */
    int startTorProxy(const char* torPath){ // torPath points to the tor.exe executable
        STARTUPINFO si = {0};
        int createProcessResult = CreateProcess((LPCSTR)torPath, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &this->torProxyProcess); // Start the proxy executable
        if(createProcessResult == 0){ // CreateProcess returns 0 on failure
            DEBUG_printf("startTorProxy(): ERR: CreateProcess failed: %d\n", GetLastError());
            return createProcessResult; // Return the result of CreateProcess so the caller is aware of the failure
        }
        DEBUG_printf("startTorProxy(): Tor proxy executable started\n");
        return createProcessResult;
    }

public:
    /*
        torSocket constructor
        Currently does nothing
    */
    torSocket(){

    }

    /*
        torSocket destructor
        Closes the socket, cleans up Winsock, and terminates the proxy process
    */
    ~torSocket(){ // Destructor
        closesocket(this->torProxySocket); // Close the socket
        WSACleanup(); // Cleanup Winsock
        TerminateProcess(this->torProxyProcess.hProcess, 0); // Terminate the proxy process
    }

    /*
        startProxy()
        Starts the Tor proxy executable and connects to it
        Arguments:
            torPath: The path to the tor.exe executable
            waitTimeSeconds: The amount of time to wait for the proxy to start (in seconds of course)
            torProxyIP: The IP address of the proxy
            torProxyPort: The port of the proxy
    */
    void startProxy(const char* torPath = ".\\tor\\tor.exe", // The path to the tor.exe executable
                    const int waitTimeSeconds = 10, // The amount of time to wait for the proxy to start - possibly not necessary and probably not the best way to wait for the proxy to start either
                    const char* torProxyIP = "127.0.0.1", // The IP address of the proxy (almost always 127.0.0.1)
                    const int torProxyPort = 9050 // The port of the proxy (almost always 9050)
                    ){
        // === Start the proxy:
        DEBUG_printf("torSocket(): Waiting for Tor proxy to start (%d seconds)...\n", waitTimeSeconds);
        int startTorProxyResult = this->startTorProxy(torPath); // Start the proxy executable as a separate process
        if(startTorProxyResult == 0){ // If the proxy failed to start, abort the connection attempt
            DEBUG_printf("torSocket(): ERR: Aborting due to failed proxy start\n");
            return;
        }
        Sleep(waitTimeSeconds * 1000); // Sleep for the specified amount of time to allow the proxy to start
        DEBUG_printf("torSocket(): Proxy is probably running, attempting to connect...\n");
        // === Initialize Winsock:
        int WSAStartupResult = WSAStartup(MAKEWORD(2, 2), &this->wsaData); // MAKEWORD(2,2) specifies version 2.2 of Winsock
        if(WSAStartupResult != 0){ // WSAStartup returns 0 on success
            DEBUG_printf("torSocket(): ERR: WSAStartup failed with error: %d\n", WSAStartupResult);
            return; // Abort the connection attempt if WSAStartup failed
        }
        // === Create a SOCKET for connecting to the proxy
        this->torProxySocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // Create a TCP socket to connect to the proxy with
        if(this->torProxySocket == INVALID_SOCKET){ // If the socket failed to create, abort the connection attempt
            DEBUG_printf("torSocket(): ERR: socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup(); // Cleanup Winsock
            return; // Abort the connection attempt
        }
        // === Setup the proxy address structure:
        this->torProxyAddr.sin_family = AF_INET; // IPv4
        this->torProxyAddr.sin_addr.s_addr = inet_addr(torProxyIP); // The IP address of the proxy but in a format that Winsock likes
        this->torProxyAddr.sin_port = htons(torProxyPort); // The port of the proxy but in a format that Winsock likes
        // === Connect to the proxy:
        int connectResult = connect(this->torProxySocket, (SOCKADDR*)&this->torProxyAddr, sizeof(this->torProxyAddr)); // Connect to the proxy using the socket and address structure defined above
        if(connectResult == SOCKET_ERROR){ // If the connection failed, abort the connection attempt
            DEBUG_printf("torSocket(): ERR: connect failed with error: %ld\n", WSAGetLastError());
            closesocket(this->torProxySocket); // Close the socket
            WSACleanup(); // Cleanup Winsock
            return; // Abort the connection attempt
        }
        DEBUG_printf("torSocket(): Connected to proxy at %s:%d\n", torProxyIP, torProxyPort);
        // === Authenticate with the proxy:
        char authReq[3] = {0x05, 0x01, 0x00}; // SOCKS5, 1 auth method, no auth
        send(this->torProxySocket, authReq, sizeof(authReq), 0); // Send the authentication request
        char authResp[2]; // A small buffer to hold the response from the proxy
        recv(this->torProxySocket, authResp, sizeof(authResp), 0); // Receive the response from the proxy
        if(authResp[1] != 0x00){ // If the proxy responded with an error, abort the connection attempt
            DEBUG_printf("torSocket(): ERR: Proxy authentication failed with error: %d\n", authResp[1]);
            closesocket(this->torProxySocket); // Close the socket
            WSACleanup(); // Cleanup Winsock
            return; // Abort the connection attempt
        }
        DEBUG_printf("torSocket(): Proxy authentication successful\n");
        this->connected = true; // We have successfully connected to the proxy and authenticated, so set this->connected to true
    }

    /*
        connectTo()
        Connects to a host through the proxy.
        Can be passed a domain name or an IP address
        Arguments:
            host: The host to connect to
            port: The port to connect to
    */
    void connectTo(const char* host, 
                   const int port=80){
        // === Initial checking and setup:
        if(!this->connected){ // If we are not connected to the proxy, abort the connection attempt and give an error message to the stupid developer who forgot to connect to the proxy
            DEBUG_printf("connectTo(): ERR: Not connected to proxy\n");
            return; // Abort the connection attempt
        }
        if(isIPv4(host) || isIPv6(host)){
            DEBUG_printf("connectTo(): ERR: Host is an IP address, not a domain name. Unsupported (might fix)\n");
            return; // Abort the connection attempt
        }
        DEBUG_printf("connectTo(): Attempting to connect to %s:%d\n", host, port);
        // === Assemble a SOCKS5 connect request:
        char hostLen = (char)strlen(host); // Get the length of the domain, as a char so it can be sent as a single byte
        char* connectReq = new char[7 + hostLen]; // 7 bytes for the SOCKS5 header, 1 byte for the domain length, and the domain itself
        connectReq[0] = 0x05; // Specify that we are using the  SOCKS5 protocol
        connectReq[1] = 0x01; // Specify that this is a connect command
        connectReq[2] = 0x00; // Reserved
        connectReq[3] = 0x03; // Specify that the host is a domain name
        connectReq[4] = hostLen; // Domain length
        memcpy(connectReq + 5, host, hostLen); // Copy the domain into the connect request, starting at byte 5
        short portN = htons(port); // Convert the port to network byte order.
        memcpy(connectReq + 5 + hostLen, &portN, 2); // Copy the port into the connect request, starting at byte <domain length> + 5
        // === Send the connect request:
        DEBUG_printf("connectTo(): Sending connect request to proxy\n");
        send(this->torProxySocket, connectReq, 7 + hostLen, 0); // Send the connect request to the proxy // TODO: ERROR CHECKING
        delete[] connectReq; // Free the memory used to store the connect request
        // === Get the connect response:
        DEBUG_printf("connectTo(): Waiting for connect response from proxy...\n");
        char connectResp[10]; // A buffer to hold the connect response from the proxy
        recv(this->torProxySocket, connectResp, sizeof(connectResp), 0); // Receive the connect response from the proxy
        if(connectResp[1] != 0x00){ // If the proxy responded with an error, abort the connection attempt
            DEBUG_printf("connectTo(): ERR: Proxy connection failed with error: %d: %s", connectResp[1], getSocks5Error(connectResp[1]));
            return; // Abort the connection attempt
        }
        DEBUG_printf("connectTo(): Successfully connected to %s:%d\n", host, port);
        this->socks5ConnectedToHost = true; // We have successfully connected to the host through the proxy, so set this->socks5ConnectedToHost to true
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
            return -1; // Return -1 to indicate an error if we are not connected to the proxy
        }
        DEBUG_printf("proxySend(): Sending %d bytes...\n", len);
        int sent = send(this->torProxySocket, data, len, 0); // Just a simple send() call! Reads <len> bytes from <data> and sends them to the proxy
        DEBUG_printf("proxySend(): Sent %d bytes\n", sent);
        return sent; // Return the number of bytes sent
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
            return -1; // Return -1 to indicate an error if we are not connected to the proxy
        }
        DEBUG_printf("proxyRecv(): Receiving (up to) %d bytes...\n", len);
        int received = recv(this->torProxySocket, data, len, 0); // Just a simple recv() call! Reads up to <len> bytes from the proxy and stores them in <data>. Data is a pointer to a buffer, so it will be modified.
        DEBUG_printf("proxyRecv(): Received %d bytes\n", received);
        return received; // Return the number of bytes received
    }

    /*
        proxyClose()
        Stops the proxy and closes the socket
    */
    void proxyClose(){
        if(this->connected){ // If we are connected to the proxy, close the socket and terminate the proxy process
            DEBUG_printf("proxyClose(): Closing proxy socket\n");
            closesocket(this->torProxySocket); // Close the socket
            DEBUG_printf("proxyClose(): Running WSACleanup\n");
            WSACleanup(); // Cleanup Winsock
            DEBUG_printf("proxyClose(): Terminating proxy process\n");
            if(TerminateProcess(this->torProxyProcess.hProcess, 0) == 0){ // Terminate the proxy process
                DEBUG_printf("proxyClose(): ERR: Failed to terminate proxy process\n");
            }
            this->connected = false; // Set this->connected to false
            this->socks5ConnectedToHost = false; // Set this->socks5ConnectedToHost to false
        }else{
            DEBUG_printf("proxyClose(): ERR: Not connected to proxy - attempting to terminate proxy process anyway\n");
            TerminateProcess(this->torProxyProcess.hProcess, 0); // Terminate the proxy process - Hopefully this doesnt break anything if the proxy process is already terminated
        }
    }

};

}