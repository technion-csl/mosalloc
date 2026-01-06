// exports.c — ISO C + GNU memalign (optional), with write(2) tracing
#include <stddef.h>   // size_t
#include <stdlib.h>   // getenv
#include <string.h>   // strlen
#include <unistd.h>   // write
#include "trace.h"

/* ---------- internal allocator entry points from src/malloc.c ---------- */
void   *__libc_malloc(size_t);
void    __libc_free(void *);
void   *__libc_calloc(size_t, size_t);
void   *__libc_realloc(void *, size_t);
void   *__libc_memalign(size_t, size_t);

/* May or may not exist in your snapshot */
void   *__libc_aligned_alloc(size_t, size_t) __attribute__((weak));

/* ---------- public wrappers interposed by LD_PRELOAD ---------- */
void *malloc(size_t n)                  { trace_msg("allocation: malloc");   return __libc_malloc(n); }
void  free(void *p)                     { trace_msg("free: free");           __libc_free(p); }
void *calloc(size_t n, size_t s)        { trace_msg("allocation: calloc");   return __libc_calloc(n, s); }
void *realloc(void *p, size_t n)        { trace_msg("allocation: realloc");  return __libc_realloc(p, n); }

/* C11: aligned_alloc. Prefer libc’s if present, else map to __libc_memalign. */
void *aligned_alloc(size_t alignment, size_t size) {
    trace_msg("allocation: aligned_alloc");
    if (__libc_aligned_alloc) return __libc_aligned_alloc(alignment, size);
    /* C11 requires size % alignment == 0; caller responsibility. */
    return __libc_memalign(alignment, size);
}

/* Optional GNU extension: memalign (NOT POSIX; keep only if you want it). */
#ifdef MALLOC_FORK_EXPORT_GNU_MEMALIGN
void *memalign(size_t alignment, size_t size) {
    trace_msg("allocation: memalign");
    return __libc_memalign(alignment, size);
}
#endif

/* Intentionally NOT exported (to avoid POSIX in the malloc API surface):
   - posix_memalign
   - valloc
   - pvalloc
   And therefore: no #include <sysconf> or page-size queries.
*/
