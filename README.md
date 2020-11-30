## Proxy Server Project (CPS 373)

### Introduction
This is the final project by Austin Butz and Phyo Thuta Aung for CPS 373: Intro to Computer Networking class at Franklin
 & Marshall College, Lancaster Pa. 

### Instructions
To use this proxy server, first clone the git repo:
```
git clone https://github.com/paung23/Proxy_Server.git
```
You can then go into the repo and do the following:
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
