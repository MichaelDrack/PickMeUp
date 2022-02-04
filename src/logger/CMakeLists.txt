cmake_minimum_required(VERSION 2.8)
project(main)

set(EXEC_SOURCES logger.cpp)

add_library(logger STATIC ${EXEC_SOURCES})
