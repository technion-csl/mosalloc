#pragma once
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* Our shim so struct mallinfo2 is always visible */
#include <malloc/malloc.h>

/* stdio lock aliases */
#ifndef _IO_flockfile
# define _IO_flockfile(f) flockfile(f)
#endif
#ifndef _IO_funlockfile
# define _IO_funlockfile(f) funlockfile(f)
#endif

/* GLRO passthrough */
#ifndef GLRO
# define GLRO(x) (x)
#endif

/* mremap symbol */
#ifndef __mremap
# define __mremap mremap
#endif


#ifndef SINGLE_THREAD_P
# define SINGLE_THREAD_P 0
#endif

/* Force __nonnull(...) to be harmless even if system headers predefine it */
#ifdef __nonnull
# undef __nonnull
#endif
#define __nonnull(x)  /* eat the parens */


#ifndef __attr_access
#  define __attr_access(x)  /* nothing */
#endif

/* TLS model attribute used in arena.c for __thread vars */
#ifndef attribute_tls_model_ie
#  define attribute_tls_model_ie /* nothing */
#endif

/* If internal typedefs don't land, provide safe fallbacks. */
#ifndef INTERNAL_SIZE_T
#  define INTERNAL_SIZE_T size_t
#endif
/* ---- opaque typedef fallbacks (needed very early) ---- */
#ifndef mstate
struct malloc_state;      typedef struct malloc_state *mstate;
#endif
#ifndef mchunkptr
struct malloc_chunk;      typedef struct malloc_chunk *mchunkptr;
#endif

/* ---- memory tagging: give a safe granule and keep tokens consistent ---- */
#ifndef __MTAG_GRANULE_SIZE
#  define __MTAG_GRANULE_SIZE 16
#endif
/* rtld “readonly” accessor: pass-through in standalone builds */
#ifndef GLRO
# define GLRO(x) (x)
#endif

/* Make the page-size symbol resolve without pulling rtld state */
#ifndef dl_pagesize
# define dl_pagesize (getpagesize())
#endif

/* Map glibc’s internal syscall wrappers to public ones */
#ifndef __mmap
# define __mmap mmap
#endif
#ifndef __munmap
# define __munmap munmap
#endif

#ifndef __mprotect
# define __mprotect mprotect
#endif
#ifndef __madvise
# define __madvise madvise
#endif
#ifndef __sbrk
# define __sbrk sbrk
#endif

/* SystemTap/SDT probes used by malloc: disable */
#ifndef LIBC_PROBE
# define LIBC_PROBE(name, nargs, ...) /* no-op */
#endif

/* Only if your headers don’t provide it; harmless otherwise */
#ifndef MREMAP_MAYMOVE
# define MREMAP_MAYMOVE 1
#endif

/* iostream flag bit sometimes touched in __malloc_stats() */
#ifndef _IO_FLAGS2_NOTCANCEL
# define _IO_FLAGS2_NOTCANCEL 0
#endif

/* Standalone build: treat process as non-secure (no setuid/sanitized-env mode). */
#ifndef __libc_enable_secure
#define __libc_enable_secure 0
#endif

#ifndef GLIBC_COMPAT_H
#define GLIBC_COMPAT_H

/* Make glibc internal annotations harmless outside the full build */
#ifndef attribute_hidden
# define attribute_hidden /* nothing */
#endif
#ifndef attribute_relro
# define attribute_relro  /* nothing */
#endif

/* glibc uses IS_IN(libc) / IS_IN(rtld) in headers; default them off */
#ifndef IS_IN
# define IS_IN(x) 0
#endif

/* Some system headers expect these to exist */
#ifndef __THROW
# define __THROW
#endif
#ifndef __extern_always_inline
# define __extern_always_inline extern inline __attribute__((__always_inline__))
#endif
/* Make symbol versioning a no-op for standalone builds */
/* --- symbol versioning: make it a no-op for standalone builds --- */
#ifndef SHLIB_COMPAT
#  define SHLIB_COMPAT(lib, introduced, obsoleted) 0
#endif
#ifndef versioned_symbol
#  define versioned_symbol(lib, local, symbol, version)
#endif
#ifndef compat_symbol
#  define compat_symbol(lib, local, symbol, version)
#endif
#ifndef default_symbol_version
#  define default_symbol_version(local, symbol, version)
#endif
#ifndef strong_alias
#  define strong_alias(sym, alias)
#endif
#ifndef weak_alias
#  define weak_alias(sym, alias)
#endif

/* Forward decl so prototypes that take struct mallinfo2* compile */
struct mallinfo2;

/* Full definition so functions can allocate/assign to it */
#ifndef GLIBC_STANDALONE_MALLINFO2
#define GLIBC_STANDALONE_MALLINFO2 1
struct mallinfo2 {
  size_t arena;     /* total space allocated from system */
  size_t ordblks;   /* number of free chunks */
  size_t smblks;    /* number of fastbin blocks */
  size_t hblks;     /* number of mmapped regions */
  size_t hblkhd;    /* space in mmapped regions */
  size_t usmblks;   /* kept for compat */
  size_t fsmblks;   /* space in fastbin free blocks */
  size_t uordblks;  /* total allocated space */
  size_t fordblks;  /* total free space */
  size_t keepcost;  /* top-most, releasable space */
};
#endif

#endif /* GLIBC_COMPAT_H */