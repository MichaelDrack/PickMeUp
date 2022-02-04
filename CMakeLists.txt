cmake_minimum_required(VERSION 2.8)
project(BusTraveler_server)

set(SOURCE_DIR src)
#set()

set(MAIN_PATH ${SOURCE_DIR}/main)

set(EXEC_SRC main.cpp)

add_library(asio INTERFACE)
target_compile_options(asio INTERFACE)
target_include_directories(asio INTERFACE ${CMAKE_SOURCE_DIR}/asio/include)

message("CMAKE_SOURCE_DIR          ${CMAKE_SOURCE_DIR}/asio/include")
include_directories(${SOURCE_DIR})					# HEADER FILES
add_executable(main ${MAIN_PATH}/${EXEC_SRC})
add_subdirectory(${SOURCE_DIR}/server)				# Добавление подпроекта, указывается имя дирректории
add_subdirectory(${SOURCE_DIR}/logger)

target_link_libraries(asio INTERFACE pthread)

target_link_libraries(main asio server logger)		# Линковка программы с библиотекой

