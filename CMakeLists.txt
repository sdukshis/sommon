cmake_minimum_required(VERSION 2.8)

project(sommon CXX)

set(CMAKE_CXX_FLAGS "-Wall -std=c++0x -D UNIX")

include_directories(./include)

add_subdirectory(examples)
