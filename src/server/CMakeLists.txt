cmake_minimum_required(VERSION 2.8)
project(server)

set(EXEC_SOURCES server.cpp)

include_directories(../../asio/include/)

#add_subdirectory(asio /home/michael/packages/asio-1.18.2 EXCLUDE_FROM_ALL)

add_library(server STATIC ${EXEC_SOURCES})
