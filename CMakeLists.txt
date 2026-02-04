cmake_minimum_required(VERSION 3.10)
project(mosalloc)

set(CMAKE_CXX_STANDARD 11)

# Option to build only malloc-standalone-automated
enable_testing()
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --verbose)

# Define macros for mosalloc
add_definitions(-DTHREAD_SAFETY)
include(CompilerFlags.cmake)

set(API_LIBRARY "${PROJECT_NAME}-api")
add_subdirectory(src)
add_subdirectory(test)
# Testing

