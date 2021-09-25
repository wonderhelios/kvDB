cmake_minimum_required(VERSION 3.20)
project(kvDB)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

aux_source_directory(. NET_SRCS)

add_executable(net_test ${NET_SRCS})
