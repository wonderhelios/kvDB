cmake_minimum_required(VERSION 3.20)
project(kvDB)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_subdirectory(src/server)
add_subdirectory(src/client)
add_subdirectory(src/test)


