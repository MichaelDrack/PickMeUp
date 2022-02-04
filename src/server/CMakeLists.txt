cmake_minimum_required(VERSION 2.8)
project(server)

set(EXEC_SOURCES server.cpp)

include_directories(../../asio/include/)

add_library(server STATIC ${EXEC_SOURCES})
