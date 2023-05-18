#include <string>
#include <stdio.h>
#include <thread>

#include "../torplusplus.hpp"

#define HOST "cryptbbtg65gibadeeo2awe3j7s6evg7eklserehqr4w4e2bis5tebid.onion"
#define HOST2 "google.com"


void httpGetThread(torPlusPlus::TORSocket s, std::string host){
    std::cout << "=== Connecting to " << host << std::endl;
    s.connectProxyTo(host.c_str(), 80); // Connect to the hidden service
    std::string httpReq = "GET / HTTP/1.1\r\nHost: " + std::string(host) + "\r\n\r\n"; // Assemble a HTTP GET request to send to the site
    std::cout << "=== Sending request to " << host << ":" << std::endl;
    s.proxySend(httpReq); // Send the request to the hidden service
    std::cout << "=== Waiting for response from " << host << ":" << std::endl;
    char* buf = (char*)calloc(50000, sizeof(char)); // Allocate a buffer to receive the response
    s.proxyRecv(buf, 50000); // Receive the response from the hidden service
    printf("%s\n", buf); // Print whatever the server sent back
    free(buf);
}

int main(){
    /*
    	* TOR setup:
    */
    torPlusPlus::TOR tor(9051, ".tpptorrc", true); // port 9051, create config file at "./tpptorrc", enable logging
                                                   // NOTE: The config file will be "cleared" (deleted), so dont accidentally set the path to anything important!
    tor.start();
    
    /*
    	* Start two threads that each send a HTTP GET request to a different site:
    */
    std::thread t1(httpGetThread, tor.getSocket(), HOST);
    std::thread t2(httpGetThread, tor.getSocket(), HOST2);
    t1.join();
    t2.join();
    
    return 0;
}