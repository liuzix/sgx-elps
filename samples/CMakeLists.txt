cmake_minimum_required(VERSION 3.1)
project(elps-samples)

set(CMAKC_FLAGS "-std=gnu99 -fPIE -Wall -Werror")
set(CMAKE_EXE_LINKER_FLAGS "-nostdlib -fPIE -static -O0")

add_executable(hello "hello.c")
target_link_libraries(hello OS)
