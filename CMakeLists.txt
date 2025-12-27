cmake_minimum_required(VERSION 3.10)
project(mosalloc)

set(CMAKE_CXX_STANDARD 11)

# enable testing
enable_testing()
# Add custom target to run tests verbosely
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --verbose)

# Define macros required by the code to enable thread-safety code
add_definitions(-DTHREAD_SAFETY)
# Include CompilerFlags cmake file to add extra warning/error compiler flags
include(CompilerFlags.cmake)

set(API_LIBRARY "${PROJECT_NAME}-api")
add_subdirectory(src)
add_subdirectory(test)

