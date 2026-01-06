# Build libmalloc_min.so with mosalloc implementation
project(malloc_min C CXX)

# Minimal malloc source files (C)
set(MINIMAL_SOURCES
    src/malloc.c
    src/exports.c
)

# Mosalloc implementation files (C++) - glob from parent directory, exclude parent's hooks.cc
file(GLOB MOSALLOC_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../*.cc")
list(FILTER MOSALLOC_SOURCES EXCLUDE REGEX ".*/hooks.cc")
# Add local hooks.cc (without #include <malloc.h>)
list(APPEND MOSALLOC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/hooks.cc)

# Common include directories
set(MALLOC_MIN_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/compat_next
    ${CMAKE_CURRENT_SOURCE_DIR}/include/glibc-src
    ${CMAKE_CURRENT_SOURCE_DIR}/include/glibc-src/sysdeps
    ${CMAKE_CURRENT_SOURCE_DIR}/include/glibc-src/elf
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

# Common compile options
set(MALLOC_MIN_COMPILE_OPTIONS
    "SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/include/config.h"
    "SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/include/glibc_compat.h"
    -fPIC
    -O2
    -pthread
    -Wall
    -Wextra
)

# Create shared library with both minimal malloc and mosalloc implementation (includes hooks.cc)
add_library(malloc_min SHARED ${MINIMAL_SOURCES} ${MOSALLOC_SOURCES})

# Create API library without hooks.cc (for testing without constructor side effects)
set(MOSALLOC_SOURCES_NO_HOOKS ${MOSALLOC_SOURCES})
list(FILTER MOSALLOC_SOURCES_NO_HOOKS EXCLUDE REGEX ".*/hooks.cc")
add_library(malloc_min_api SHARED ${MINIMAL_SOURCES} ${MOSALLOC_SOURCES_NO_HOOKS})

# Apply common settings to both targets
foreach(target malloc_min malloc_min_api)
    target_include_directories(${target} PUBLIC ${MALLOC_MIN_INCLUDE_DIRS})
    target_compile_definitions(${target} PRIVATE _GNU_SOURCE)
    target_compile_options(${target} PRIVATE ${MALLOC_MIN_COMPILE_OPTIONS})
    set_target_properties(${target} PROPERTIES CXX_STANDARD 11)
    target_link_libraries(${target} PRIVATE pthread dl)
    set_target_properties(${target} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../"
    )
endforeach()

# Set C standard for C files
set_source_files_properties(${MINIMAL_SOURCES} PROPERTIES COMPILE_FLAGS "-std=gnu11")