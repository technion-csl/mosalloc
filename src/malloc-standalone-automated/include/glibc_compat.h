#ifndef GLIBC_COMPAT_H
#define GLIBC_COMPAT_H 1

/* Minimal glibc compatibility helpers used by malloc.c / arena.c and friends.
   This file is force-included via -include on every compile. */

#include <stddef.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>      /* memset, memcpy, ... */
#include <unistd.h>      /* sbrk, getpagesize */
#include <sys/types.h>   /* off_t, intptr_t */
#include <sys/mman.h>    /* mmap, munmap, mprotect, madvise, MADV_* */
#include <sys/sysinfo.h> /* get_nprocs */

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/*  Branch prediction helpers                                         */
/* ------------------------------------------------------------------ */

#ifndef __glibc_unlikely
# define __glibc_unlikely(x) __builtin_expect(!!(x), 0)
#endif

#ifndef __glibc_likely
# define __glibc_likely(x) __builtin_expect(!!(x), 1)
#endif

/* ------------------------------------------------------------------ */
/*  Compiler feature helpers                                          */
/* ------------------------------------------------------------------ */

#ifndef __GNUC_PREREQ
# if defined __GNUC__ && defined __GNUC_MINOR__
#  define __GNUC_PREREQ(maj, min) \
    (__GNUC__ > (maj) || (__GNUC__ == (maj) && __GNUC_MINOR__ >= (min)))
# else
#  define __GNUC_PREREQ(maj, min) 0
# endif
#endif

#ifndef __glibc_clang_prereq
# if defined __clang_major__ && defined __clang_minor__
#  define __glibc_clang_prereq(maj, min, patch) \
    (__clang_major__ > (maj) || \
     (__clang_major__ == (maj) && \
      (__clang_minor__ > (min) || \
       (__clang_minor__ == (min) && __clang_patchlevel__ >= (patch)))))
# else
#  define __glibc_clang_prereq(maj, min, patch) 0
# endif
#endif

#ifndef __glibc_has_builtin
# ifdef __has_builtin
#  define __glibc_has_builtin(x) __has_builtin(x)
# else
#  define __glibc_has_builtin(x) 0
# endif
#endif

/* glibc internal "which library are we building?" helper.
   For our single .so, we just treat everything as "in libc". */
/* Try to mimic glibc’s IS_IN(lib) logic enough for malloc.c. */
#ifndef IS_IN
# define IS_IN(lib) IS_IN_##lib

/* We want to behave as if we are being built into libc, not the loader. */
# define IS_IN_libc      0
# define IS_IN_rtld      0
# define IS_IN_libpthread 0
# define IS_IN_librt     0
# define IS_IN_libdl     0

#endif

/* ------------------------------------------------------------------ */
/*  Attributes / visibility                                           */
/* ------------------------------------------------------------------ */

#ifndef attribute_hidden
# define attribute_hidden __attribute__ ((visibility ("hidden")))
#endif

#ifndef __attribute_const__
# define __attribute_const__ __attribute__ ((__const__))
#endif

#ifndef __attribute_maybe_unused__
# define __attribute_maybe_unused__ __attribute__ ((__unused__))
#endif

#ifndef __attribute_noinline__
# define __attribute_noinline__ __attribute__ ((__noinline__))
#endif

#ifndef __always_inline
# define __always_inline __inline__ __attribute__ ((__always_inline__))
#endif

/* Extra attribute helpers that malloc.c / headers may use. */

#ifndef __attribute_malloc__
# define __attribute_malloc__ /* nothing */
#endif

#ifndef __attribute_alloc_size__
# define __attribute_alloc_size__(params) /* nothing */
#endif

#ifndef __attribute_warn_unused_result__
# define __attribute_warn_unused_result__ /* nothing */
#endif

#ifndef __attribute_used__
# define __attribute_used__ /* nothing */
#endif

#ifndef __attr_dealloc
# define __attr_dealloc(f, arg) /* nothing */
#endif

/* IMPORTANT: neuter __nonnull completely.
   This turns "fn(...) __nonnull (1)" into just "fn(...)" */
#ifdef __nonnull
# undef __nonnull
#endif
#define __nonnull(params)

/* glibc-style throw macros, leave them mostly empty. */

#ifndef __THROW
# define __THROW
#endif

#ifndef __THROWNL
# define __THROWNL __THROW
#endif

#ifndef __LEAF
# define __LEAF
#endif

#ifndef __LEAF_ATTR
# define __LEAF_ATTR
#endif

#ifndef __NTH
# define __NTH(fct) fct __THROW
#endif

/* TLS model attribute used in arena.c:
   "static __thread mstate thread_arena attribute_tls_model_ie;" */
#ifndef attribute_tls_model_ie
# define attribute_tls_model_ie
#endif

/* ------------------------------------------------------------------ */
/*  Probes (SystemTap etc.) – noop or stub                            */
/* ------------------------------------------------------------------ */

/* Simple no-op; stap-probe.h will redefine this, which is fine
   (you'll just get a warning about redefinition). */
#ifndef LIBC_PROBE
# define LIBC_PROBE(name, nargs, ...) /* no-op */
#endif

/* ------------------------------------------------------------------ */
/*  Versioning / visibility helpers as no-ops                         */
/* ------------------------------------------------------------------ */

#ifndef libc_hidden_def
# define libc_hidden_def(name) /* nothing */
#endif

#ifndef libc_hidden_weak
# define libc_hidden_weak(name) /* nothing */
#endif

#ifndef libc_hidden_proto
# define libc_hidden_proto(name, ...)  /* nothing */
#endif

#ifndef strong_alias
# define strong_alias(sym, alias) /* nothing */
#endif

#ifndef weak_alias
# define weak_alias(sym, alias) /* nothing */
#endif

#ifndef weak_function
# define weak_function __attribute__ ((weak))
#endif

#ifndef versioned_symbol
# define versioned_symbol(lib, local, symbol, version)  /* nothing */
#endif

#ifndef compat_symbol
# define compat_symbol(lib, local, symbol, version)     /* nothing */
#endif

#ifndef default_symbol_version
# define default_symbol_version(local, symbol, version) /* nothing */
#endif

/* Used in _itoa.h etc. */
#ifndef hidden_proto
# define hidden_proto(name) /* no-op for standalone malloc */
#endif

/* ------------------------------------------------------------------ */
/*  Lock type & macros (fallback if libc-lock.h shim not included)    */
/* ------------------------------------------------------------------ */

#ifndef LLL_LOCK_INITIALIZER
# define LLL_LOCK_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#endif

#ifndef _LIBC_LOCK_INITIALIZER
# define _LIBC_LOCK_INITIALIZER LLL_LOCK_INITIALIZER
#endif

#ifndef __libc_lock_t
typedef pthread_mutex_t __libc_lock_t;
#endif

#ifndef __libc_lock_define
# define __libc_lock_define(CLASS, NAME) \
    CLASS __libc_lock_t NAME
#endif

#ifndef __libc_lock_define_initialized
# define __libc_lock_define_initialized(CLASS, NAME) \
    CLASS __libc_lock_t NAME = _LIBC_LOCK_INITIALIZER
#endif

#ifndef __libc_lock_init
# define __libc_lock_init(NAME) \
    (pthread_mutex_init (&(NAME), NULL))
#endif

#ifndef __libc_lock_lock
# define __libc_lock_lock(NAME) \
    (pthread_mutex_lock (&(NAME)))
#endif

#ifndef __libc_lock_unlock
# define __libc_lock_unlock(NAME) \
    (pthread_mutex_unlock (&(NAME)))
#endif

#ifndef __libc_lock_trylock
# define __libc_lock_trylock(NAME) \
    (pthread_mutex_trylock (&(NAME)))
#endif

#ifndef __libc_lock_destroy
# define __libc_lock_destroy(NAME) \
    (pthread_mutex_destroy (&(NAME)))
#endif

/* ------------------------------------------------------------------ */
/*  Security flag stub                                                */
/* ------------------------------------------------------------------ */

/* glibc normally sets this for setuid/setgid binaries; for standalone
   malloc we can safely treat it as "not secure" (0). */
#ifndef __libc_enable_secure
# define __libc_enable_secure 0
#endif

/* ------------------------------------------------------------------ */
/*  GLRO / dl_pagesize glue                                           */
/* ------------------------------------------------------------------ */

/* We don't want full rtld_global_ro; just make GLRO(dl_pagesize)
   act as getpagesize() for our purposes. */
#ifndef GLRO
# define GLRO(name) (glibc_compat_get_##name())
#endif

static inline size_t
glibc_compat_get_dl_pagesize (void)
{
    return (size_t) getpagesize();
}

/* ------------------------------------------------------------------ */
/*  __madvise / __mmap / __munmap / __mprotect wrappers               */
/* ------------------------------------------------------------------ */

#ifndef __madvise
static inline int
__madvise (void *addr, size_t length, int advice)
{
    return madvise (addr, length, advice);
}
#endif

#ifndef __mmap
static inline void *
__mmap (void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
    return mmap (addr, length, prot, flags, fd, offset);
}
#endif

#ifndef __munmap
static inline int
__munmap (void *addr, size_t length)
{
    return munmap (addr, length);
}
#endif

#ifndef __mprotect
static inline int
__mprotect (void *addr, size_t length, int prot)
{
    return mprotect (addr, length, prot);
}
#endif

#ifndef __get_nprocs
static inline int
__get_nprocs (void)
{
    return get_nprocs ();
}
#endif

/* ------------------------------------------------------------------ */
/*  mstate forward-declare so prototypes compile                      */
/* ------------------------------------------------------------------ */

/* If glibc's malloc.c typedef is not seen yet, make sure mstate is a
   pointer to an (incomplete) struct malloc_state so prototypes like
   _int_malloc(mstate, size_t) parse correctly. */

#ifndef mstate
struct malloc_state;
typedef struct malloc_state *mstate;
#endif

/* ------------------------------------------------------------------ */
/*  __sbrk wrapper                                                    */
/* ------------------------------------------------------------------ */

#ifndef __sbrk
static inline void *
__sbrk (intptr_t increment)
{
    return sbrk (increment);
}
#endif

/* ------------------------------------------------------------------ */
/*  mallinfo2 compatibility                                           */
/* ------------------------------------------------------------------ */

/* Host libc may not define struct mallinfo2; malloc.c expects a full
   definition with these fields (used only for stats). We provide a
   minimal compatible struct. */
#ifndef GLIBC_STANDALONE_MALLINFO2
#define GLIBC_STANDALONE_MALLINFO2 1
struct mallinfo2
{
  size_t arena;
  size_t ordblks;
  size_t smblks;
  size_t hblks;
  size_t hblkhd;
  size_t usmblks;
  size_t fsmblks;
  size_t uordblks;
  size_t fordblks;
  size_t keepcost;
};
#endif

#ifdef __cplusplus
}
#endif

#endif /* GLIBC_COMPAT_H */
