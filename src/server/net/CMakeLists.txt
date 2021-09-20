cmake_minimum_required(VERSION 3.20)
project(moonDB)

set(CMAKE_CXX_STANDARD 14)

aux_source_directory(. NET_SRCS)

add_executable(net_test ${NET_SRCS})
