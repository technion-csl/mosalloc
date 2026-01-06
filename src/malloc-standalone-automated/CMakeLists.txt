# Build libmalloc_auto.so with mosalloc implementation
project(malloc_auto C CXX)

# ============================================================
# Step 1: Run make to generate malloc.c and apply mosalloc patch
# ============================================================
set(MALLOC_AUTO_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(MALLOC_C ${MALLOC_AUTO_DIR}/src/malloc.c)
set(MORECORE_C ${MALLOC_AUTO_DIR}/src/morecore.c)

# Custom command to generate and patch malloc.c
add_custom_command(
    OUTPUT ${MALLOC_C} ${MORECORE_C}
    COMMAND make mosalloc
    WORKING_DIRECTORY ${MALLOC_AUTO_DIR}
    COMMENT "Generating malloc.c from glibc and applying mosalloc __morecore patch"
    VERBATIM
)

# Custom target to trigger generation
add_custom_target(generate_malloc_auto
    DEPENDS ${MALLOC_C} ${MORECORE_C}
)

# ============================================================
# Step 2: Build shared library with mosalloc implementation
# ============================================================

# Automated malloc source files (C)
set(AUTO_SOURCES
    src/malloc.c
    src/exports.c
    src/malloc-hugepages.c
    src/errno_shim.c
)

# Mosalloc implementation files (C++) - glob from parent directory, exclude parent's hooks.cc
file(GLOB MOSALLOC_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../*.cc")
list(FILTER MOSALLOC_SOURCES EXCLUDE REGEX ".*/hooks.cc")
# Add local hooks.cc (we'll create one without #include <malloc.h>)
list(APPEND MOSALLOC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/hooks.cc)

# Common include directories (matching CPPFLAGS from build/CPPFLAGS)
set(MALLOC_AUTO_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/special_shims
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/glibc-src
    ${CMAKE_CURRENT_SOURCE_DIR}/include/glibc-src/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/glibc-src/sysdeps
    ${CMAKE_CURRENT_SOURCE_DIR}/include/glibc-src/bits
    ${CMAKE_CURRENT_SOURCE_DIR}/include/glibc-src/malloc
    ${CMAKE_CURRENT_SOURCE_DIR}/include/glibc-src/stdlib
    ${CMAKE_CURRENT_SOURCE_DIR}/include/special_shims/elf
    ${CMAKE_CURRENT_SOURCE_DIR}/include/glibc-src/sysdeps/generic
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

# Common compile options (matching Makefile flags)
set(MALLOC_AUTO_COMPILE_OPTIONS
    "SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/include/config.h"
    "SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/include/auto_stubs.h"
    "SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/include/glibc_compat.h"
    -fPIC
    -O2
    -pthread
    -Wall
    -Wextra
)

# Create shared library with both automated malloc and mosalloc implementation (includes hooks.cc)
add_library(malloc_auto SHARED ${AUTO_SOURCES} ${MOSALLOC_SOURCES})
add_dependencies(malloc_auto generate_malloc_auto)

# Create API library without hooks.cc (for testing without constructor side effects)
set(MOSALLOC_SOURCES_NO_HOOKS ${MOSALLOC_SOURCES})
list(FILTER MOSALLOC_SOURCES_NO_HOOKS EXCLUDE REGEX ".*/hooks.cc")
add_library(malloc_auto_api SHARED ${AUTO_SOURCES} ${MOSALLOC_SOURCES_NO_HOOKS})
add_dependencies(malloc_auto_api generate_malloc_auto)

# Apply common settings to both targets
foreach(target malloc_auto malloc_auto_api)
    target_include_directories(${target} PUBLIC ${MALLOC_AUTO_INCLUDE_DIRS})
    target_compile_definitions(${target} PRIVATE _GNU_SOURCE)
    target_compile_options(${target} PRIVATE ${MALLOC_AUTO_COMPILE_OPTIONS})
    set_target_properties(${target} PROPERTIES CXX_STANDARD 11)
    target_link_libraries(${target} PRIVATE pthread dl)
    set_target_properties(${target} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../"
    )
endforeach()

# Set C standard for C files
set_source_files_properties(${AUTO_SOURCES} PROPERTIES COMPILE_FLAGS "-std=gnu11")
