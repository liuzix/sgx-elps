cmake_minimum_required(VERSION 3.1)
project(elps-samples)

set(CMAKC_FLAGS "-std=gnu99 -Wall -Werror")
set(CMAKE_EXE_LINKER_FLAGS "-nostdlib -ffreestanding -fPIC -static -O0")

add_executable(hello "hello.c")
target_link_libraries(hello OS)
