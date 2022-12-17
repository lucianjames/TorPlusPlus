#include <string>

#include "torplusplus.hpp"

#define TEST_ONION "cryptbbtg65gibadeeo2awe3j7s6evg7eklserehqr4w4e2bis5tebid.onion"
// The above test address is for the hacking forum CryptBB

int main(){
    torSocketGlobals::DEBUG = true; // Enable debug messages
    torSocket torSock;
    torSock.connectTo(TEST_ONION);
    std::string httpReq = "GET / HTTP/1.1\r\nHost: " + std::string(TEST_ONION) + "\r\n\r\n";
    torSock.proxySend(httpReq.c_str(), httpReq.length());
    char buf[16384] = {0}; // 16KB buffer
    int recvResult = torSock.proxyRecv(buf, sizeof(buf) * sizeof(char));
    printf("%s\n", buf);
    return 0;
}
