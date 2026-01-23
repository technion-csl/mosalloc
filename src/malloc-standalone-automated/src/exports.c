// exports.c — public malloc API wrappers for standalone glibc malloc
// Interposed via LD_PRELOAD
//
// This file intentionally stays *simple*:
//  - no RTLD_NEXT
//  - no libc fallbacks
//  - call glibc internals directly
//
// If this file works, crashes are NOT caused by the wrappers.

#include <stddef.h>   // size_t, ptrdiff_t
#include <unistd.h>   // write
#include "trace.h"
#include <stdatomic.h>
#include <stdint.h>

extern  void __ptmalloc_init(void);

static atomic_int g_malloc_inited = 0;
static atomic_int g_malloc_init_in_progress = 0;

static inline void ensure_ptmalloc_init(void)
{
    if (atomic_load_explicit(&g_malloc_inited, memory_order_acquire))
        return;

    // startup is effectively single-threaded, but be safe:
    int was = atomic_exchange_explicit(&g_malloc_init_in_progress, 1, memory_order_acq_rel);
    if (was == 0) {
        if (__ptmalloc_init)
            __ptmalloc_init();
        atomic_store_explicit(&g_malloc_inited, 1, memory_order_release);
    } else {
        while (!atomic_load_explicit(&g_malloc_inited, memory_order_acquire)) {
            __asm__ __volatile__("pause");
        }
    }
}


/* ------------------------------------------------------------ */
/* Internal allocator entry points from glibc malloc.c          */
/* ------------------------------------------------------------ */

void *__libc_malloc (size_t);
void  __libc_free (void *);
void *__libc_calloc (size_t, size_t);
void *__libc_realloc (void *, size_t);
void *__libc_memalign (size_t, size_t);
int __libc_mallopt (int, int);

/* May or may not exist depending on glibc version */
void *__libc_aligned_alloc (size_t, size_t) __attribute__((weak));

/* ------------------------------------------------------------ */
/* Public malloc-family symbols (LD_PRELOAD interposition)      */
/* ------------------------------------------------------------ */

void *
malloc (size_t n)
{
    ensure_ptmalloc_init();
    trace_msg("[mf] wrapper: malloc\n");
    return __libc_malloc (n);
}

void
free (void *p)
{
    ensure_ptmalloc_init();
    trace_msg("[mf] wrapper: free\n");
    __libc_free (p);
}

void *
calloc (size_t n, size_t s)
{
    ensure_ptmalloc_init();
    trace_msg("[mf] wrapper: calloc\n");
    return __libc_calloc (n, s);
}

void *
realloc (void *p, size_t n)
{
    ensure_ptmalloc_init();
    trace_msg("[mf] wrapper: realloc\n");
    return __libc_realloc (p, n);
}

__attribute__((visibility("default")))
int
mallopt (int param, int value)
{
    ensure_ptmalloc_init();
    trace_msg("[mf] wrapper: mallopt\n");
    return __libc_mallopt (param, value);
}
/* ------------------------------------------------------------ */
/* C11 aligned_alloc                                            */
/* ------------------------------------------------------------ */

void *
aligned_alloc (size_t alignment, size_t size)
{
    trace_msg("[mf] wrapper: aligned_alloc\n");

    if (__libc_aligned_alloc)
        return __libc_aligned_alloc (alignment, size);

    /* Fallback for older glibc snapshots */
    return __libc_memalign (alignment, size);
}

/* ------------------------------------------------------------ */
/* GNU extension (optional)                                     */
/* ------------------------------------------------------------ */
#ifdef MALLOC_FORK_EXPORT_GNU_MEMALIGN
void *
memalign (size_t alignment, size_t size)
{
    trace_msg("[mf] wrapper: memalign\n");
    return __libc_memalign (alignment, size);
}
#endif