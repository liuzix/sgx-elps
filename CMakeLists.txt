cmake_minimum_required(VERSION 3.1)
project(sgx-elps)

add_subdirectory(swapper)
add_subdirectory(loader)
add_subdirectory(libOS)
add_subdirectory(samples)
add_subdirectory(test)
