# TorPlusPlus
Allows a C++ program to send and receive data through TOR to regular or hidden services.
Creates and kills the TOR process itself.
Future versions/forks/branches/whatever of this code will probably let you embed TOR and everything into the program itself (antivirus probably wont like that, will essentially be a dropper).
Will probably also add automatic downloading of the TOR files instead of having to put them in there manually (and suffer from potentially outdated versions!)

## Still in development!
This project only started existing a couple hours ago, lots of functionality is yet to be implemented.

## Platform support
Currently, I have this header set up for windows exclusively. Will add linux support in the future™

## Why?
Hidden services are actually really easy to set up, but talking to them from a program is a bit more complicated. This library makes it easy to talk to a hidden service from a program.
Hidden services have the following advantages over regular websites:
- Anonymity for both parties
- No need to pay for a domain name, onion addresses are free forever!
- You dont need to port forward when hosting a hidden service
- ISPs and even governments cant block hidden services (easily)

## ❗ You need to manually do this: ❗
Requires the "tor" folder from the "Tor Expert Bundle" to be placed next to the executable.
(This file can be downloaded from https://www.torproject.org/download/tor/)

The steps to do this are:
1. Download the "Tor Expert Bundle" from https://www.torproject.org/download/tor/ (Likely Windows (x86_64) unless you are a caveman)
2. Extract the tar.gz file
3. Copy the "tor" folder (which contains the tor.exe executable) to the same folder as your executable
4. Run your executable - it should start the tor proxy and connect to it!

You can specify the path to the tor.exe executable in the torSocket constructor if you want to place it somewhere else.


I might make some kind of script to automate doing this, I dont want to just place the files in here as they will become outdated.


## ❗ Problems: ❗
* Havent got IPv4 addresses working in the connectTo() function yet
* + more that I probably havent discovered yet

## Public functions
```c++
/*
    torSocket contructor
    Starts the Tor proxy executable and connects to it
    Arguments:
        torPath: The path to the tor.exe executable
        waitTimeSeconds: The amount of time to wait for the proxy to start
        torProxyIP: The IP address of the proxy
        torProxyPort: The port of the proxy
*/
torSocket(const char* torPath = ".\\tor\\tor.exe", const int waitTimeSeconds = 10, const char* torProxyIP = "127.0.0.1", const int torProxyPort = 9050)

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
```

## Examples
See https://github.com/LJ3D/TorPlusPlusExamples for some example code


## ⚠️ Security ⚠️
⚠️ Absolutely no guarantees on security/anonymity when using this code to talk to TOR ⚠️
