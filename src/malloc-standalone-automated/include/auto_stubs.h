#ifndef AUTO_STUBS_H
#define AUTO_STUBS_H

/* Manually maintained small shims for standalone malloc. */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/* ------------------------------------------------------------------ */
/* 1. Thread-related helpers                                          */
/* ------------------------------------------------------------------ */

/*
 * In glibc, SINGLE_THREAD_P often consults TLS/thread state
 * (THREAD_GETMEM / THREAD_SELF / etc.).
 *
 * In a standalone LD_PRELOAD allocator we *must not* depend on that.
 * The safest behavior is to always take the "multi-threaded" path
 * (locks enabled) -> i.e., SINGLE_THREAD_P should be false.
 *
 * NOTE: glibc headers like sysdep-cancel.h may define SINGLE_THREAD_P.
 * We only define it if it's missing.
 */
#ifndef SINGLE_THREAD_P
# define SINGLE_THREAD_P 0
#endif

/*
 * __set_errno(e): implemented as a real function in src/errno_shim.c
 * Do NOT also define a macro __set_errno(e) here, or you’ll conflict.
 */
#ifndef AUTO_STUBS_HAVE_SET_ERRNO_DECL
#define AUTO_STUBS_HAVE_SET_ERRNO_DECL 1
extern void __set_errno (int);
#endif

/* ------------------------------------------------------------------ */
/* 2. Tcache helper                                                   */
/* ------------------------------------------------------------------ */

/*
 * Some snapshots reference tcache_init from tcache_free_init.
 * If it isn't visible, stub it as a no-op (effectively disables tcache init).
 */
#ifndef tcache_init
static inline void tcache_init (void *ptr)
{
    (void) ptr;
}
#endif

/* ------------------------------------------------------------------ */
/* 3. Libio / stdio internals used by __malloc_stats                  */
/* ------------------------------------------------------------------ */

/*
 * The real glibc code uses internal libio hooks to mark stderr as
 * "not cancellable" while printing malloc stats.
 * For standalone, map to public flockfile/funlockfile and ignore flags2.
 */
#ifndef _IO_flockfile
# define _IO_flockfile(stream) flockfile(stream)
#endif

#ifndef _IO_funlockfile
# define _IO_funlockfile(stream) funlockfile(stream)
#endif

#ifndef _IO_FLAGS2_NOTCANCEL
# define _IO_FLAGS2_NOTCANCEL 0
#endif

/* ------------------------------------------------------------------ */
/* 4. Error-reporting helpers used by malloc_printerr                 */
/* ------------------------------------------------------------------ */

/*
 * Some glibc malloc.c paths call __libc_message("%s\n", str),
 * others call __libc_fatal(str). Provide best-effort fallbacks.
 */
#ifndef __libc_message
static inline void __libc_message (const char *fmt, const char *arg)
{
    /* Best-effort: print to stderr. */
    fprintf(stderr, fmt, arg);
}
#endif

#ifndef __libc_fatal
static inline void __libc_fatal (const char *msg)
{
    /* Best-effort: log + abort to match "fatal" semantics. */
    fprintf(stderr, "%s\n", msg);
    abort();
}
#endif

/*
 * Some builds reference malloc_printerr_tail() but it may be
 * compiled out / hidden under feature macros.
 * Provide a stub that reports and returns.
 */
#ifndef HAVE_MALLOC_PRINTRERR_TAIL_STUB
#define HAVE_MALLOC_PRINTRERR_TAIL_STUB 1
static inline void malloc_printerr_tail (const char *str)
{
    __libc_message("%s\n", str);
}
#endif

#endif /* AUTO_STUBS_H */
