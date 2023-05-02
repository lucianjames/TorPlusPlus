
#include <string>
#include <stdio.h>
#include <thread>
#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS // Disable the "deprecated" warning for inet_addr()
// I tried using inet_pton() like the warning suggested, but it didnt work and I dont really care to fix it
#include <winsock2.h>
#include <ws2tcpip.h>
#include <strsafe.h>
#include <windows.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define closesocket close
#define SOCKET_ERROR    -1
#define INVALID_SOCKET  -1
#define WSACleanup() // Define cleanup to nothing on Linux
#define WSAGetLastError() errno
#define SOCKADDR struct sockaddr
typedef int SOCKET;
#endif

#include "../torplusplus.hpp"

#define HOST "cryptbbtg65gibadeeo2awe3j7s6evg7eklserehqr4w4e2bis5tebid.onion"

void clientRecvThread(SOCKET sock){
    while(true){
        char buf[1024];
        int len = recv(sock, buf, 1024, 0);
        if(len <= 0){
            printf("=== Client disconnected\n");
            break;
        }
        printf("%s", buf);
    }
}

int main(){
    torPlusPlus::TOR tor(9051, ".tpptorrc", true); // port 9051, create config file at ".tpptorrc", enable logging
    // NOTE: The config file will be "cleared" (deleted), so dont accidentally set the path to anything important!
    // Create a service that listens on port 80 and forwards all traffic to port 8081
    tor.addService("./testService", 80, 8081);
    tor.start();
#ifdef _WIN32
        WSADATA wsaData = {0};
        int WSAStartupResult = WSAStartup(MAKEWORD(2, 2), &wsaData); // MAKEWORD(2,2) specifies version 2.2 of Winsock
#endif
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8081);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    listen(sock, 5);
    while(true){
        SOCKET clientSock = accept(sock, NULL, NULL);
        printf("=== Client connected\n");
        std::thread t(clientRecvThread, clientSock);
        t.detach();
    }
    return 0;
}
