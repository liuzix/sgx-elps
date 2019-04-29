cmake_minimum_required(VERSION 3.1)
project(elps-samples)

set(CMAKE_C_FLAGS "-g -std=gnu99 -fpie -Wall -O0")
set(CMAKE_EXE_LINKER_FLAGS "-g -nostdlib -static-pie -O0")
SET(EXTRA_OBJS ../musl/lib/crti.o ../musl/lib/crtn.o)
set_source_files_properties(${EXTRA_OBJS} PROPERTIES EXTERNAL_OBJECT true GENERATED true)
include_directories(../include ../deps)

set_source_files_properties(${EXTRA_OBJS} PROPERTIES EXTERNAL_OBJECT true GENERATED true)

add_executable(testpthread "testpthread.cpp" ${EXTRA_OBJS})
target_link_libraries(testpthread OS)

add_executable(hello "testhint.cpp" ${EXTRA_OBJS})
target_link_libraries(hello OS)
