cmake_minimum_required(VERSION 3.10)
project(mosalloc)

set(CMAKE_CXX_STANDARD 11)

# Build malloc-standalone-minimal FIRST


# Define macros for mosalloc
add_definitions(-DTHREAD_SAFETY)
include(CompilerFlags.cmake)

add_subdirectory(src/malloc-standalone-minimal)
# Build mosalloc library SECOND (depends on malloc_min)
set(API_LIBRARY "${PROJECT_NAME}-api")
add_subdirectory(src)

# Testing
enable_testing()
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --verbose)
add_subdirectory(test)

