#pragma comment(lib, "ws2_32.lib")

#define _WINSOCK_DEPRECATED_NO_WARNINGS // Disable the "deprecated" warning for inet_addr()
// I tried using inet_pton() like the warning suggested, but it didnt work and I dont really care to fix it

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <strsafe.h>


// Hacky way to enable/disable debug messages, at least its hidden away in a namespace
namespace torSocketGlobals{
    bool DEBUG = false;
    #define DEBUG_printf(...) if(DEBUG) printf(__VA_ARGS__)
}
using namespace torSocketGlobals; // So we can just use DEBUG_printf() instead of torSocketGlobals::DEBUG_printf() in this file

// The torSocket class
class torSocket{
private:
    PROCESS_INFORMATION torProxyProcess;
    WSADATA wsaData;
    SOCKET torProxySocket;
    sockaddr_in torProxyAddr;
    bool connected = false;

int startTorProxy(const char* torPath){ // torPath points to the tor.exe executable
    STARTUPINFO si = {0};
    int createProcessResult = CreateProcess((LPCSTR)torPath, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &this->torProxyProcess);
    if(createProcessResult == 0){
        DEBUG_printf("startTorProxy(): ERR: CreateProcess failed: %d\n", GetLastError());
        return createProcessResult;
    }
    DEBUG_printf("startTorProxy(): Tor proxy executable started\n");
    return createProcessResult;
}

public:
    torSocket(const char* torProxyIP = "127.0.0.1",
              const int torProxyPort = 9050,
              const int waitTimeSeconds = 10,
              const char* torPath = ".\\tor\\tor.exe"){
        DEBUG_printf("torSocket(): Waiting for Tor proxy to start (%d seconds)...\n", waitTimeSeconds);
        int startTorProxyResult = this->startTorProxy(torPath);
        if(startTorProxyResult == 0){
            DEBUG_printf("torSocket(): ERR: Aborting due to failed proxy start\n");
            return;
        }
        Sleep(waitTimeSeconds * 1000); // Sleep for the specified amount of time to allow the proxy to start
        DEBUG_printf("torSocket(): Proxy is probably running, attempting to connect...\n");
        // Initialize Winsock
        int WSAStartupResult = WSAStartup(MAKEWORD(2, 2), &this->wsaData);
        if(WSAStartupResult != 0){
            DEBUG_printf("torSocket(): ERR: WSAStartup failed: %d\n", WSAStartupResult);
            return;
        }
        // Create a SOCKET for connecting to the proxy
        this->torProxySocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if(this->torProxySocket == INVALID_SOCKET){
            DEBUG_printf("torSocket(): ERR: socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return;
        }
        // Setup the proxy address structure
        this->torProxyAddr.sin_family = AF_INET;
        this->torProxyAddr.sin_addr.s_addr = inet_addr(torProxyIP);
        this->torProxyAddr.sin_port = htons(torProxyPort);
        // Connect to the proxy
        int connectResult = connect(this->torProxySocket, (SOCKADDR*)&this->torProxyAddr, sizeof(this->torProxyAddr));
        if(connectResult == SOCKET_ERROR){
            DEBUG_printf("torSocket(): ERR: connect failed with error: %ld\n", WSAGetLastError());
            closesocket(this->torProxySocket);
            WSACleanup();
            return;
        }
        DEBUG_printf("torSocket(): Connected to proxy at %s:%d\n", torProxyIP, torProxyPort);
        // Authenticate to the proxy
        char authReq[3] = {0x05, 0x01, 0x00}; // SOCKS5, 1 auth method, no auth
        send(this->torProxySocket, authReq, sizeof(authReq), 0);
        char authResp[2];
        recv(this->torProxySocket, authResp, sizeof(authResp), 0);
        if(authResp[1] != 0x00){
            DEBUG_printf("torSocket(): ERR: Proxy authentication failed with error: %d\n", authResp[1]);
            closesocket(this->torProxySocket);
            WSACleanup();
            return;
        }
        DEBUG_printf("torSocket(): Proxy authentication successful\n");
        this->connected = true;
    }

    ~torSocket(){
        closesocket(this->torProxySocket);
        WSACleanup();
        TerminateProcess(this->torProxyProcess.hProcess, 0);
    }

    void connectTo(const char* host, const int port=80){
        if(!this->connected){
            DEBUG_printf("connectTo(): ERR: Not connected to proxy\n");
            return;
        }
        short portN = htons(port); // Convert the port to network byte order
        DEBUG_printf("connectTo(): Attempting to connect to %s:%d\n", host, port);
        // Assemble a SOCKS5 connect request
        char domainLen = (char)strlen(host); // Get the length of the domain, as a char so it can be sent as a single byte
        char* connectReq = new char[7 + domainLen]; // 7 bytes for the SOCKS5 header, 1 byte for the domain length, and the domain itself
        connectReq[0] = 0x05; // SOCKS5
        connectReq[1] = 0x01; // Connect
        connectReq[2] = 0x00; // Reserved
        connectReq[3] = 0x03; // Domain
        connectReq[4] = domainLen; // Domain length
        memcpy(connectReq + 5, host, domainLen); // Domain
        memcpy(connectReq + 5 + domainLen, &portN, 2); // Port
        // Send the connect request
        DEBUG_printf("connectTo(): Sending connect request to proxy\n");
        send(this->torProxySocket, connectReq, 7 + domainLen, 0);
        delete[] connectReq; // Free the memory used to store the connect request
        // Get the connect response
        DEBUG_printf("connectTo(): Waiting for connect response from proxy...\n");
        char connectResp[10];
        recv(this->torProxySocket, connectResp, sizeof(connectResp), 0);
        if(connectResp[1] != 0x00){
            DEBUG_printf("connectTo(): ERR: Proxy connection failed with error: %d\n", connectResp[1]);
            return;
        }
        DEBUG_printf("connectTo(): Successfully connected to %s:%d\n", host, port);
    }

    int proxySend(const char* data, const int len){
        if(!this->connected){
            DEBUG_printf("proxySend(): ERR: Not connected to proxy\n");
            return -1;
        }
        DEBUG_printf("proxySend(): Sending %d bytes...\n", len);
        int sent = send(this->torProxySocket, data, len, 0);
        DEBUG_printf("proxySend(): Sent %d bytes\n", sent);
        return sent;
    }

    int proxyRecv(char* data, const int len){
        if(!this->connected){
            DEBUG_printf("proxyRecv(): ERR: Not connected to proxy\n");
            return -1;
        }
        DEBUG_printf("proxyRecv(): Receiving (up to) %d bytes...\n", len);
        int received = recv(this->torProxySocket, data, len, 0);
        DEBUG_printf("proxyRecv(): Received %d bytes\n", received);
        return received;
    }

};
