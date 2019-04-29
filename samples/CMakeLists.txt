cmake_minimum_required(VERSION 3.1)
project(elps-samples)

set(CMAKE_C_FLAGS "-std=gnu99 -fpie -Wall -O0")
set(CMAKE_EXE_LINKER_FLAGS "-nostdlib -static-pie -O0")
SET(EXTRA_OBJS ../musl/lib/crti.o ../musl/lib/crtn.o)
include_directories(../include ../deps)

add_executable(hello "testhint.cpp" ${EXTRA_OBJS})
set_source_files_properties(${EXTRA_OBJS} PROPERTIES EXTERNAL_OBJECT true GENERATED true)
target_link_libraries(hello OS)

add_executable(testpthread "testpthread.cpp" ${EXTRA_OBJS})
target_link_libraries(testpthread OS)
