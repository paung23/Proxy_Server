cmake_minimum_required(VERSION 3.1)
project(Proxy_Server)

find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROXY_SERVER_FILES
  src/ProxyServer.cpp
)

add_executable(ProxyServer ${PROXY_SERVER_FILES})
target_include_directories(ProxyServer PRIVATE third_party/asio-1.18.0/include)
target_link_libraries(ProxyServer PRIVATE Threads::Threads)
