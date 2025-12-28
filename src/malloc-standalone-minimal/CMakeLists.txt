# Build libmalloc_min.so
project(malloc_min C)

# Source files (only malloc.c; arena.c is included from it)
set(SOURCES
    src/malloc.c
    src/exports.c
    src/morecore.c
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

# Preprocessor definitions (avoid parentheses in add_definitions, use string form)
target_compile_definitions(malloc_min PRIVATE
    _GNU_SOURCE
    "SHLIB_COMPAT(a,b,c)=0"
    "versioned_symbol(a,b,c,d)="
    "compat_symbol(a,b,c,d)="
    "default_symbol_version(a,b,c)="
    "strong_alias(a,b)="
    "weak_alias(a,b)="
    "__nonnull(x)="
    "__nonnull(x)="
)

# Force include config headers
target_compile_options(malloc_min PRIVATE
    "-include" "${CMAKE_CURRENT_SOURCE_DIR}/include/config.h"
    "-include" "${CMAKE_CURRENT_SOURCE_DIR}/include/glibc_compat.h"
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