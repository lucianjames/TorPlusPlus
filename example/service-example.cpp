/*

#include <string>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>

#include "../torplusplus.hpp"

#define HOST "cryptbbtg65gibadeeo2awe3j7s6evg7eklserehqr4w4e2bis5tebid.onion"

void clientRecvThread(int sock){
    while(true){
        char buf[1024];
        int len = recv(sock, buf, 50000, 0);
        if(len <= 0){
            printf("=== Client disconnected\n");
            break;
        }
        printf("%s", buf);
    }
}

int main(){
    torPlusPlus::TOR tor(9051, ".tpptorrc", true); // port 9051, create config file at "./tpptorrc", enable logging
    // NOTE: The config file will be "cleared" (deleted), so dont accidentally set the path to anything important!
    // Create a service that listens on port 80 and forwards all traffic to port 8081
    tor.addService("./testService", 80, 8081);
    tor.start();
    // Create a socket that listens on port 8081 and prints anything it receives to stdout
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8081);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    listen(sock, 5);
    while(true){
        int clientSock = accept(sock, NULL, NULL);
        printf("=== Client connected\n");
        std::thread t(clientRecvThread, clientSock);
        t.detach();
    }
    return 0;
}

*/

#include <iostream>
int main(){
    std::cout << "Hello world" << std::endl;
    return 0;
}
