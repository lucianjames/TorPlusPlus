# TorPlusPlus
C++ library for talking to a TOR hidden service.
## You need to manually do this:
Requires the "tor" folder from the "Tor Expert Bundle" to be placed next to the executable.
(This file can be downloaded from https://www.torproject.org/download/tor/)


I might make some kind of script to automate doing this, I dont want to just place the files in here as they will become outdated.

# Platform support
Currently, I have this header set up for windows exclusively. Will add linux support in the future™

# Public functions
```c++
/*
    torSocket contructor
    Starts the Tor proxy executable and connects to it
    Arguments:
        torProxyIP: The IP address of the proxy
        torProxyPort: The port of the proxy
        waitTimeSeconds: The amount of time to wait for the proxy to start
        torPath: The path to the tor.exe executable
*/
torSocket(const char* torProxyIP = "127.0.0.1", // The IP address of the proxy (almost always 127.0.0.1)
            const int torProxyPort = 9050, // The port of the proxy (almost always 9050)
            const int waitTimeSeconds = 10, // The amount of time to wait for the proxy to start
            const char* torPath = ".\\tor\\tor.exe" // The path to the tor.exe executable
            )

/*
    torSocket destructor
    Closes the socket, cleans up Winsock, and terminates the proxy process
*/
~torSocket()

/*
    connectTo()
    Connects to a host through the proxy
    Arguments:
        host: The host to connect to
        port: The port to connect to
*/
void connectTo(const char* host, const int port=80)

/*
    proxySend()
    Sends data through the proxy to the service we are connected to
    Arguments:
        data: The data to send
        len: The length of the data to send
    Returns:
        The number of bytes sent (or -1 if an error occurred)
*/
int proxySend(const char* data, const int len)

/*
    proxyRecv()
    Receives data from the proxy
    Arguments:
        data: A pointer to a buffer to store the received data
        len: The maximum number of bytes to receive
    Returns:
        The number of bytes received (or -1 if an error occurred)
*/
int proxyRecv(char* data, const int len)

# Example
The following example code sends a HTTP GET request to "CryptBB", and prints the result.
```c++
#include <string>
#include <stdio.h>

#include "torplusplus.hpp"

#define TEST_ONION "cryptbbtg65gibadeeo2awe3j7s6evg7eklserehqr4w4e2bis5tebid.onion"

int main(){
    torSocketGlobals::DEBUG = true; // Enable debug messages
    torSocket torSock; // Create a torSocket object. Doing this starts the TOR proxy and connects to it
    torSock.connectTo(TEST_ONION); // Connect the proxy to the onion address
    std::string httpReq = "GET / HTTP/1.1\r\nHost: " + std::string(TEST_ONION) + "\r\n\r\n"; // Assemble a HTTP GET request to send to the site
    torSock.proxySend(httpReq.c_str(), (int)httpReq.length()); // Send the request to the hidden service
    char buf[16384] = {0}; // Up to 16KB of memory for whatever gets sent back
    torSock.proxyRecv(buf, sizeof(buf) * sizeof(char)); // Receive a response to the GET request
    printf("%s\n", buf); // Print whatever the server sent back
    return 0;
}
```

# ⚠️ Security ⚠️
⚠️ Absolutely no guarantees on security/anonymity when using this code to talk to TOR ⚠️
