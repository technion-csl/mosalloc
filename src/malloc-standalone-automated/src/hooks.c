/* Minimal malloc hooks stub for standalone malloc build.
   glibc's malloc.c includes "hooks.c" to define these globals.
   We don't actually use the hooks, so everything defaults to NULL. */

#include <stddef.h>
#include <malloc.h>

/* Make sure the macro exists (on glibc it does). If not, fall back to nothing. */
#ifndef __MALLOC_HOOK_VOLATILE
# define __MALLOC_HOOK_VOLATILE
#endif

/* These must exactly match the declarations in <malloc.h>.  */

void (*__MALLOC_HOOK_VOLATILE __malloc_initialize_hook) (void) = NULL;

void *(*__MALLOC_HOOK_VOLATILE __malloc_hook) (size_t __size,
                                               const void *__caller) = NULL;

void *(*__MALLOC_HOOK_VOLATILE __realloc_hook) (void *__ptr, size_t __size,
                                                const void *__caller) = NULL;

void *(*__MALLOC_HOOK_VOLATILE __memalign_hook) (size_t __alignment,
                                                 size_t __size,
                                                 const void *__caller) = NULL;

void (*__MALLOC_HOOK_VOLATILE __free_hook) (void *__ptr,
                                            const void *__caller) = NULL;

void (*__MALLOC_HOOK_VOLATILE __after_morecore_hook) (void) = NULL;
