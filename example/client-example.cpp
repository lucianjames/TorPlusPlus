#include <string>
#include <stdio.h>

#include "../torplusplus.hpp"

#define HOST "cryptbbtg65gibadeeo2awe3j7s6evg7eklserehqr4w4e2bis5tebid.onion"

int main(){
    torPlusPlus::TOR tor(9051, ".tpptorrc", true, "tor.exe"); // port 9051, create config file at "./tpptorrc", enable logging
    // NOTE: The config file will be "cleared" (deleted), so dont accidentally set the path to anything important!
    tor.start(); // Start the tor binary
    torPlusPlus::TORSocket ts = tor.getSocket(); // Get a pre-configured socket that is already connected to the "tor" instance
    ts.connectProxyTo(HOST, 80); // Connect to the hidden service
    std::string httpReq = "GET / HTTP/1.1\r\nHost: " + std::string(HOST) + "\r\n\r\n"; // Assemble a HTTP GET request to send to the site
    ts.proxySend(httpReq); // Send the request to the hidden service
    char* buf = (char*)calloc(50000, sizeof(char)); // Allocate a buffer to receive the response
    ts.proxyRecv(buf, 50000); // Receive the response from the hidden service
    printf("%s\n", buf); // Print whatever the server sent back
    free(buf);
    tor.stop(); // This happens when the class is destroyed, but you can do it manually too :)
    return 0;
}