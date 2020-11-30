## Proxy Server Project (CPS 373)

### Introduction
This is the final project by Austin Butz and Phyo Thuta Aung for CPS 373: Intro to Computer Networking class at Franklin
 & Marshall College, Lancaster Pa. 

### Instructions
To use this proxy server, first clone the git repo:
```
git clone https://github.com/paung23/Proxy_Server.git
```

In the client web browser you will be using, please go into the network settings and set up the manual proxy 
configuration. Enter "localhost" or "127.0.0.1" for hostname (or a custom IP address if you want but you will need to
change that in the code as well) and "8001" (or any port number you want to use) for port. Depending on which browser 
you are using, you will also need to go into the configuration and make sure three things: (1) the HTTP version is 1.0, 
(2) the network HTTP proxy keepalive is turned off, and (3) the traffic pipelining is turned off.

We have used Firefox (v.82.0.3) as the client web browser while developing this proxy server, and so, in Firefox, you can 
set up the above three things in 'about:config'.

Afterwards, you can then go into the repo and do the following:
```
mkdir build
cd build
cmake ..
make
```
You can now run the proxy from the build directory by calling (if the port number is not specified, the proxy server 
will listen on 8001):
```
./ProxyServer <PortNumber - optional>
```
For content filtering functionality, if you would like to make your own set of blacklist, you can go to 
[data/blacklist.txt](https://github.com/paung23/Proxy_Server/blob/master/data/blacklist.txt) and add one website per 
line. Those websites will then be blacklisted by the proxy server.

For logging, you can check [data/log.txt](https://github.com/paung23/Proxy_Server/blob/master/data/log.txt) for your own
 record of network traffic.

### Codebase
The main file is at [src/ProxyServer.cpp](https://github.com/paung23/Proxy_Server/blob/master/src/ProxyServer.cpp). We 
have imported Asio library for listening to requests from client browser and for making HTTP requests to servers.
