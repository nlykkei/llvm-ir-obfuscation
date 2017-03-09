cmake_minimum_required(VERSION 3.5.1)

project("LLVMOPasses")

set(CMAKE_CXX_STANDARD 11)

find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

add_subdirectory(flatten)  # Use your pass name here.
add_subdirectory(merge)
add_subdirectory(add)
add_subdirectory(cyclomatic)
