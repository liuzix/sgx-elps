cmake_minimum_required(VERSION 3.10)
project (UnitTest)
 
# Locate GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(../include)
 
# Link testall with what we want to test and the GTest and pthread library
add_executable(testall test.cpp)
target_link_libraries(testall ${GTEST_LIBRARIES} pthread)
