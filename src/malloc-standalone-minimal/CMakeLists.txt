# Build libmalloc_min.so
project(malloc_min C)

# Source files (only malloc.c; arena.c and morecore.c are included from it)
set(SOURCES
    src/malloc.c
    src/exports.c
)

# Create shared library FIRST
add_library(malloc_min SHARED ${SOURCES})

# Include directories
target_include_directories(malloc_min PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compat_next
  ${CMAKE_CURRENT_SOURCE_DIR}/include/glibc-src
  ${CMAKE_CURRENT_SOURCE_DIR}/include/glibc-src/sysdeps
  ${CMAKE_CURRENT_SOURCE_DIR}/include/glibc-src/elf
)

# Preprocessor definitions
# Note: Function-style macros (SHLIB_COMPAT, versioned_symbol, etc.) are already
# defined in glibc_compat.h which is force-included below
target_compile_definitions(malloc_min PRIVATE
    _GNU_SOURCE
)

# Force include config headers
target_compile_options(malloc_min PRIVATE
    "SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/include/config.h"
    "SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/include/glibc_compat.h"
    -fPIC
    -O2
    -pthread
    -std=gnu11
    -Wall
    -Wextra
)

# Linker flags
target_link_libraries(malloc_min PRIVATE
    pthread
    dl
)