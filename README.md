# See the rewrite-linux-windows branch!
https://github.com/LJ3D/TorPlusPlus/tree/rewrite-linux-windows

# TorPlusPlus
Allows a C++ program to send and receive data through TOR, either to regular clearweb services or to hidden services via onion addresses.
Also contains functionality for quickly setting up hidden services, allowing you to make a program which configures and starts the hidden service for itself automatically.

## Still in development!
This new version adds in some extra functionality, and isnt 100% tested.

## Platform support
Supports both Linux and Windows, with features being the same on both.

## Why?
Hidden services are actually really easy to set up, but talking to them from a program is a bit more complicated. This library makes it easy to talk to a hidden service from a program.
Hidden services have the following advantages over regular websites:
- Anonymity for both parties
- No need to pay for a domain name, onion addresses are free forever!
- You dont need to port forward when hosting a hidden service
- ISPs and even governments cant block hidden services (easily)

## Examples (outdated!)
See https://github.com/LJ3D/TorPlusPlusExamples for some example code (OUTDATED EXAMPLES - WILL FIX)

See https://github.com/LJ3D/TorRemoteAccess/ for a more functional program utilising TorPlusPlus (OUTDATED - PROBABLY WONT FIX)

## ❗ You need to manually do this: ❗
TOR must be either installed and accessible from the command line, or an executable for TOR must be placed alongside your executable.
The `torPlusPlus::TOR.start()` and `torPlusPlus::TOR.startFromFile()` functions are used depending on which method of using TOR you choose.
If you are calling an executable directly, you must pass the path to it into the constructor for the `TOR` class


## Public functions / Overviews of TOR and TORSocket classes

TODO

## ⚠️ Security ⚠️
No guarantees :)
