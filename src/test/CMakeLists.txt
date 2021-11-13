cmake_minimum_required(VERSION 3.20)

aux_source_directory(. SRCS)

add_executable(test ${SRCS} ../client/DBClient.cpp)
