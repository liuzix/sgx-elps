cmake_minimum_required(VERSION 3.10)
ENABLE_LANGUAGE(ASM-ATT)
SET(CMAKE_CXX_FLAGS "-g -O0 -Wall -fpie -Werror -std=c++17 -fno-omit-frame-pointer -fno-stack-protector -fno-use-cxa-atexit -Wno-frame-address")
SET(Boost_USE_STATIC_LIBS ON)
project(libOS)

include_directories(../include)
include_directories(../deps)

file(GLOB SOURCES "*.cpp" "*.S" "*.s")
find_package(Boost REQUIRED COMPONENTS context)

add_library(OSCore STATIC ${SOURCES})
target_compile_definitions(OSCore PUBLIC IS_LIBOS)
target_compile_definitions(OSCore PUBLIC BOOST_NO_EXCEPTIONS)

find_library(LIB_MUSL libc.a PATHS "../musl/lib" NO_DEFAULT_PATH)
execute_process(COMMAND ${CMAKE_C_COMPILER} -print-libgcc-file-name OUTPUT_VARIABLE GCC_LIB)
get_filename_component(GCC_DIR "${GCC_LIB}" DIRECTORY)
find_library(GCC_LIBSTDCPP libstdc++.a PATHS "${GCC_DIR}" NO_DEFAULT_PATH)
find_library(GCC_LIBGCC_EH libgcc_eh.a PATHS "${GCC_DIR}" NO_DEFAULT_PATH)
find_library(GCC_LIBSUPCPP libsupc++.a PATHS "${GCC_DIR}" NO_DEFAULT_PATH)

set(OS_LIB ${PROJECT_SOURCE_DIR}/buildtmp/libOS.a)
file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/buildtmp)
# We need to pack all standard libraries needed
add_custom_target(OS_COMBINE
    COMMAND ar -x $<TARGET_FILE:OSCore>
    COMMAND ar -x ${LIB_MUSL}
    COMMAND ar -x /usr/lib/x86_64-linux-gnu/libboost_context.a
    COMMAND ar -x ../libstdc++.a
    COMMAND ar -x ${GCC_LIBGCC_EH}
    COMMAND ar -qcs ${OS_LIB} *.o *.lo
    WORKING_DIRECTORY ./buildtmp
    DEPENDS OSCore
)

add_library(OS STATIC IMPORTED GLOBAL)
add_dependencies(OS OS_COMBINE)
set_target_properties(OS PROPERTIES IMPORTED_LOCATION ${OS_LIB})
