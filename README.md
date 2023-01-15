# TorPlusPlus
Allows a C++ program to send and receive data through TOR to regular or hidden services.
Creates and kills the TOR process itself.
Future versions/forks/branches/whatever of this code will probably let you embed TOR and everything into the program itself (antivirus probably wont like that, will essentially be a dropper).
Will probably also add automatic downloading of the TOR files instead of having to put them in there manually (and suffer from potentially outdated versions!)

## Still in early development!
Reasonable potential for bugs and missing features!

## Platform support
Supports both Linux and Windows, but Linux does not yet have to capability to start TOR! TOR must already be running on the system.

## Why?
Hidden services are actually really easy to set up, but talking to them from a program is a bit more complicated. This library makes it easy to talk to a hidden service from a program.
Hidden services have the following advantages over regular websites:
- Anonymity for both parties
- No need to pay for a domain name, onion addresses are free forever!
- You dont need to port forward when hosting a hidden service
- ISPs and even governments cant block hidden services (easily)

## Examples
See https://github.com/LJ3D/TorPlusPlusExamples for some example code

## ❗ You need to manually do this: ❗
### Windows:
Requires the "tor" folder from the "Tor Expert Bundle" to be placed next to the executable.

The steps to do this are:
1. Download the "Tor Expert Bundle" from https://www.torproject.org/download/tor/ (Likely Windows (x86_64) unless you are a 32 bit caveman)
2. Extract the tar.gz file
3. Copy the "tor" folder (which contains the tor.exe executable) to the same folder as your executable
4. Run your executable - it should start the tor proxy and connect to it!

You can specify the path to the tor.exe executable in the torSocket constructor if you want to place it somewhere else.

You may find https://github.com/LJ3D/TorRemoteAccess/tree/master/TRAClient interesting if you dont want to do this!

### Linux:
TOR must already be running on the system.
**Making TorPlusPlus start TOR on Linux is TODO**

The steps to do this are:
1. Install TOR via you favourite package manager (for example, `pacman -S tor`)
2. Start TOR by running `sudo tor`
3. (Optional) Make TOR run as a service on startup so you dont have to manually start it every time you want to use your program. `sudo systemctl enable tor.service` (Not sure if this is a good idea or not)

## Public functions of the torSocket class
```c++
/*
    torSocket constructor
    Currently does nothing
*/
torSocket()


/*
    torSocket destructor
    Closes the socket, cleans up Winsock, and terminates the proxy process
*/
~torSocket()


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


/*
    connectToProxy()
    Connects to the Tor proxy
    Arguments:
        torProxyIP: The IP address of the proxy (almost always 127.0.0.1)
        torProxyPort: The port of the proxy (almost always 9050)
*/
void connectToProxy(const char* torProxyIP = "127.0.0.1",
                    const int torProxyPort = 9050)


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


/*
    connectProxyTo()
    Connects to a host through the proxy.
    Can be passed a domain name or an IP address
    Arguments:
        host: The host to connect to
        port: The port to connect to
*/
void connectProxyTo(const char* host, 
                    const int port=80)


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


 /*
    closeTorSocket()
    Stops the proxy and closes the socket
*/
void closeTorSocket()

```

## ⚠️ Security ⚠️
⚠️ Absolutely no guarantees on security/anonymity when using this code to talk to TOR ⚠️
