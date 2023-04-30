# TorPlusPlus
Allows a C++ program to send and receive data through TOR, either to regular clearweb services or to hidden services via onion addresses.
Theoretically, can be used with other SOCKS5 proxies, but only if theyre configured exactly the same way the TOR SOCKS5 proxy is.

## Still in development!
I want to expand the functionality of this header, especially the `torSocketExtended` class.

## Platform support
Linux+Windows

## Why?
Hidden services are actually really easy to set up, but talking to them from a program is a bit more complicated. This library makes it easy to talk to a hidden service from a program.
Hidden services have the following advantages over regular websites:
- Anonymity for both parties
- No need to pay for a domain name, onion addresses are free forever!
- You dont need to port forward when hosting a hidden service
- ISPs and even governments cant block hidden services (easily)

## Examples
See https://github.com/LJ3D/TorPlusPlusExamples for some example code

See https://github.com/LJ3D/TorRemoteAccess/ for a more functional program utilising TorPlusPlus

## ❗ You need to manually do this: ❗
### Windows:
Requires the "tor" folder from the "Tor Expert Bundle" to be placed next to the executable.

The steps to do this are:
1. Download the "Tor Expert Bundle" from https://www.torproject.org/download/tor/ (Likely Windows (x86_64) unless you are a 32 bit caveman)
2. Extract the tar.gz file
3. Copy the "tor" folder (which contains the tor.exe executable) to the same folder as your executable
4. Run your executable - it should start the tor proxy and connect to it!

You can specify the path to the tor.exe executable in the torSocket constructor if you want to place it somewhere else.

See https://github.com/LJ3D/TorRemoteAccess/tree/master/TRAClient for a windows visual studio project that embeds tor.exe inside itself as a resource.

### Linux:
TOR must already be installed on the system.

The steps to do this are:
1. Install TOR via you favourite package manager (for example, `pacman -S tor`)
2. Start TOR by running `sudo tor`, or `sudo systemctl start tor`. You can use `sudo systemctl enable tor` to cause tor to start at boot.
3. OR use the `startTorProxy()` or `startAndConnectToProxy()` functions

## Public functions of the torSocket class
```c++
/*
    startTorProxy()
    Starts the Tor proxy executable as a separate process
    Arguments:
        torPath: The path to the tor.exe executable. Defaults to ".\\tor\\tor.exe"
    Returns:
        1 if the proxy process was started successfully
        0 if the proxy process failed to start
*/
int startTorProxy(const char* torPath = ".\\tor\\tor.exe")
```
```c++
/*
    connectToProxy()
    Connects to the Tor proxy
    Arguments:
        torProxyIP: The IP address of the proxy (almost always 127.0.0.1)
        torProxyPort: The port of the proxy (almost always 9050)
    Returns:
        1 on success
        0 on error
*/
int connectToProxy(const char* torProxyIP = "127.0.0.1",
                   const int torProxyPort = 9050)
```
```c++
/*
    startAndConnectToProxy()
    Combines the startTorProxy() and connectToProxy() functions into one function for convenience
    Arguments:
        torPath: The path to the tor.exe executable
        torProxyIP: The IP address of the proxy (almost always 127.0.0.1)
        torProxyPort: The port of the proxy (almost always 9050)
    Returns:
        1 on success, 0 on failure. Uses the value of this->connected to determine success or failure if this->startTorProxy() returns 1
*/
int startAndConnectToProxy(const char* torPath = "\\tor\\tor.exe",
                           const char* torProxyIP = "127.0.0.1",
                           const int torProxyPort = 9050)
```
```c++
/*
    connectProxyTo()
    Connects to a host through the proxy.
    Can be passed a domain name or an IP address
    Arguments:
        host: The host to connect to
        port: The port to connect to
    Returns:
        SOCKS5 err code (0 on success, see RFC1928 section 6)
        Returns -1 on other failures
*/
int connectProxyTo(const char* host, 
                   const int port=80)
```
```c++
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
              const int len)
```
```c++
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
              const int len)
```
```c++
/*
    closeTorSocket()
    Stops the proxy and closes the socket
*/
void closeTorSocket()
```
```c++
/*
    getSocket()
    Returns the socket used to connect to the proxy, allowing you to use the socket directly (in case you need to do something that this class doesn't support)
*/
SOCKET getSocket()

```

## ⚠️ Security ⚠️
No guarantees :)
