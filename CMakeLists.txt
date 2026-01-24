cmake_minimum_required(VERSION 3.10)
project(mosalloc)

set(CMAKE_CXX_STANDARD 11)

# Option to build only malloc-standalone-automated
option(MALLOC_AUTO_ONLY "Build only malloc-standalone-automated" OFF)

enable_testing()
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --verbose)

# Define macros for mosalloc
add_definitions(-DTHREAD_SAFETY)
include(CompilerFlags.cmake)

if(NOT MALLOC_AUTO_ONLY)
  # Build mosalloc library (depends on malloc_min)
  set(API_LIBRARY "${PROJECT_NAME}-api")
  add_subdirectory(src)
endif()

add_subdirectory(src/malloc-standalone-automated)
add_subdirectory(test)
# Testing

