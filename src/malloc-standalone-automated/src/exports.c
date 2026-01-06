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

/* ------------------------------------------------------------ */
/* Internal allocator entry points from glibc malloc.c          */
/* ------------------------------------------------------------ */

void *__libc_malloc (size_t);
void  __libc_free (void *);
void *__libc_calloc (size_t, size_t);
void *__libc_realloc (void *, size_t);
void *__libc_memalign (size_t, size_t);

/* May or may not exist depending on glibc version */
void *__libc_aligned_alloc (size_t, size_t) __attribute__((weak));

/* Exported by morecore.c included in malloc.c */
extern void *__glibc_morecore (ptrdiff_t);

/* ------------------------------------------------------------ */
/* Public malloc-family symbols (LD_PRELOAD interposition)      */
/* ------------------------------------------------------------ */

void *
malloc (size_t n)
{
    trace_msg("[mf] wrapper: malloc\n");
    return __libc_malloc (n);
}

void
free (void *p)
{
    trace_msg("[mf] wrapper: free\n");
    __libc_free (p);
}

void *
calloc (size_t n, size_t s)
{
    trace_msg("[mf] wrapper: calloc\n");
    return __libc_calloc (n, s);
}

void *
realloc (void *p, size_t n)
{
    trace_msg("[mf] wrapper: realloc\n");
    return __libc_realloc (p, n);
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

/* ------------------------------------------------------------ */
/* morecore — explicitly exported                               */
/* ------------------------------------------------------------ */
/*
 * This symbol is NOT part of POSIX or ISO C.
 * We export it intentionally for experimentation / research.
 */
void *
morecore (ptrdiff_t increment)
{
    trace_msg("[mf] wrapper: morecore\n");
    return __glibc_morecore (increment);
}
