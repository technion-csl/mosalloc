#ifndef MALLOC_FORK_LDSODEFS_H_SHIM
#define MALLOC_FORK_LDSODEFS_H_SHIM

#include <stddef.h>

/* Opaque stand-ins so malloc.c / arena.c compile.
   We never dereference these in the standalone build.  */
struct link_map;
struct r_debug;

/* Dummy dl_relocate_ld predicate: always "already relocated".  */
static inline int
dl_relocate_ld (const struct link_map *l __attribute__((unused)))
{
  return 1;
}

/* We don't care about l_soname in standalone builds.  */
static inline const char *
l_soname (const struct link_map *l __attribute__((unused)))
{
  return NULL;
}

/* Any other ld.so globals / macros can be added here if they pop up,
   but so far malloc.c/arena.c don't need the real loader state. */

#endif /* MALLOC_FORK_LDSODEFS_H_SHIM */
