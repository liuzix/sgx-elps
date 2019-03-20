cmake_minimum_required(VERSION 3.1)
project(elps-samples)

set(CMAKE_C_FLAGS "-std=gnu99 -fPIE -Wall")
set(CMAKE_EXE_LINKER_FLAGS "-nostdlib -pie -static -O0")

add_executable(hello "hello.c")
target_link_libraries(hello OS)
