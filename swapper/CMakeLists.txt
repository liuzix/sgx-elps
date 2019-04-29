cmake_minimum_required(VERSION 3.1)
SET(CMAKE_CXX_FLAGS "-g -O0 -Wall -Werror -std=c++17")
project(swapper)

include_directories(../include)
include_directories(../deps)

file(GLOB SOURCES "*.cpp")

add_library(swapper SHARED ${SOURCES})
