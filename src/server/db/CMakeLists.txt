cmake_minimum_required(VERSION 3.20)
project(moonDB)

set(CMAKE_CXX_STANDARD 14)

aux_source_directory(. DB_SRCS)

add_executable(db_test ${DB_SRCS})
