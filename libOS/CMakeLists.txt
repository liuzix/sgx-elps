cmake_minimum_required(VERSION 3.10)
ENABLE_LANGUAGE(ASM-ATT)
SET(CMAKE_CXX_FLAGS "-g -Wall -Werror -std=c++17")

project(libOS)

include_directories(../include)

file(GLOB SOURCES "*.cpp" "*.s")

add_library(OS STATIC ${SOURCES})
