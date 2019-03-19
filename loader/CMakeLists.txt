cmake_minimum_required(VERSION 3.10)
SET(CMAKE_CXX_FLAGS "-g -Wall -Werror -std=c++17 -fPIC")
ENABLE_LANGUAGE(ASM-ATT)
project(loader)

include_directories(../include)
include_directories(../deps)

include(FindOpenSSL)
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

include(FindProtobuf)
find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIR})

PROTOBUF_GENERATE_CPP(PROTO_SRC PROTO_HEADER aesm.proto)

file(GLOB SOURCES "*.cpp")

add_executable(loader ${SOURCES} entry.s ${PROTO_SRC})
target_link_libraries(loader ${PROTOBUF_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY} swapper)


