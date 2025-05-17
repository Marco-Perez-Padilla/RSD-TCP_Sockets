cmake_minimum_required(VERSION 3.13)
project(ftp_server)

find_package(Threads REQUIRED)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x ")
add_executable(ftp_server ftp_server.cpp FTPServer.cpp ClientConnection.cpp)
target_link_libraries(ftp_server PRIVATE Threads::Threads)
