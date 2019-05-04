cmake_minimum_required(VERSION 3.1)
project(elps-samples)

set(CMAKE_C_FLAGS "-g -std=gnu99 -fpie -Wall -O0")
set(CMAKE_EXE_LINKER_FLAGS "-g -nostdlib -static-pie -O0")
SET(EXTRA_OBJS ../musl/lib/crti.o ../musl/lib/crtn.o)
set_source_files_properties(${EXTRA_OBJS} PROPERTIES EXTERNAL_OBJECT true GENERATED true)
include_directories(../include ../deps)

add_executable(client "client.c" ${EXTRA_OBJS})
target_link_libraries(client OS)

add_executable(testpthread "testpthread.cpp" ${EXTRA_OBJS})
target_link_libraries(testpthread OS)

add_executable(hello "testhint.cpp" ${EXTRA_OBJS})
target_link_libraries(hello OS)

#add_subdirectory(libevent)
#add_subdirectory(memcached)

include(ExternalProject)
set(SAMPLES_LDFLAGS "-g -static-pie -nostdlib -L../libevent/.libs -L../../libOS/buildtmp -Wl,-Bstatic,--start-group -lOS ")
set(SAMPLES_CFLAGS "-g -I../../musl/include -I../../musl/include -I../../musl/obj/include -I../../musl/arch/x86_64 -I../../musl/arch/generic -nostdinc -Wno-error=parentheses -Wno-error=redundant-decls -pie -w -U_FORTIFY_SOURCE")

ExternalProject_Add(elps-libevent
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libevent
    CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/libevent/configure --disable-openssl --disable-gcc-warnings --disable-shared CFLAGS=${SAMPLES_CFLAGS} CC=gcc --disable-libevent-regress --disable-samples
    BUILD_COMMAND make clean && make all
    BUILD_IN_SOURCE TRUE
    BUILD_ALWAYS TRUE
    INSTALL_COMMAND "")
add_dependencies(elps-libevent musl OS)

ExternalProject_Add(elps-memcached
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/memcached"
    CONFIGURE_COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/memcached && ./autogen.sh COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/memcached/configure --host=x86_64-linux-gnu --disable-coverage LDFLAGS=${SAMPLES_LDFLAGS} "CFLAGS=-U_FORTIFY_SOURCE -I../libevent/include -pie -g" CC=gcc ac_cv_c_endian=little
    BUILD_IN_SOURCE TRUE
    BUILD_ALWAYS TRUE
    BUILD_COMMAND make clean && make
    INSTALL_COMMAND "")
add_dependencies(elps-memcached musl elps-libevent OS)
