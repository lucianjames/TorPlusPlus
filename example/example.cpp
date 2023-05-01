#include <string>
#include <stdio.h>

#include "../torplusplus.hpp"

#define HOST "cryptbbtg65gibadeeo2awe3j7s6evg7eklserehqr4w4e2bis5tebid.onion"

int main(){
    torPlusPlus::TOR tor(9050, "./tpptorrc", true);
    tor.start();
    torPlusPlus::TORSocket ts = tor.getSocket();
    ts.connectProxyTo(HOST, 80); // Connect to the hidden service
    std::string httpReq = "GET / HTTP/1.1\r\nHost: " + std::string(HOST) + "\r\n\r\n"; // Assemble a HTTP GET request to send to the site
    ts.proxySend(httpReq.c_str(), (int)httpReq.length()); // Send the request to the hidden service
    char buf[16384] = {0}; // Up to 16KB of memory for whatever gets sent back
    ts.proxyRecv(buf, sizeof(buf) * sizeof(char)); // Receive a response to the GET request
    printf("%s\n", buf); // Print whatever the server sent back
    return 0;
}