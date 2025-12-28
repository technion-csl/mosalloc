#pragma once
/*
 * glibc_shim.h
 *
 * Unified, minimal shim for building glibc malloc.c out-of-tree.
 * This file intentionally keeps only what ptmalloc needs in a
 * standalone build, and puts every symbol behind an #ifndef guard.
 *
 * It merges the following light-weight shims:
 *   - libc-internal.h
 *   - libc-pointer-arith.h
 *   - not-cancel.h
 *   - random-bits.h
 *   - libc-mtag.h
 *   - setvmaname.h
 *   - shlib-compat.h
 *
 * If you later decide to split again, each section is clearly marked.
 */

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

/* sys/random.h is optional; use it if present */
#if defined(__has_include)
#  if __has_include(<sys/random.h>)
#    include <sys/random.h>
#    define __STANDALONE_HAVE_SYS_RANDOM 1
#  endif
#endif

/* ================================================================
 * Section: libc-internal.h (minimal)
 * ================================================================ */
#ifndef attribute_hidden
# define attribute_hidden
#endif
#ifndef attribute_relro
# define attribute_relro
#endif
#ifndef attribute_noreturn
# define attribute_noreturn __attribute__((__noreturn__))
#endif
#ifndef attribute_unused
# define attribute_unused __attribute__((__unused__))
#endif

#ifndef __glibc_likely
# define __glibc_likely(x)   __builtin_expect(!!(x),1)
#endif
#ifndef __glibc_unlikely
# define __glibc_unlikely(x) __builtin_expect(!!(x),0)
#endif

#ifndef __set_errno
# define __set_errno(e) (errno = (e))
#endif

#ifndef __libc_fatal
static inline attribute_noreturn void __libc_fatal(const char *msg)
{
  if (msg) {
    size_t n = 0; while (msg[n] != '\0') n++;
    (void)!write(2, msg, n);
    (void)!write(2, "\n", 1);
  }
  _exit(127);
}
#endif

#ifndef __libc_message
static inline void __libc_message(int flags, const char *fmt, ...)
{
  (void)flags;
  va_list ap; va_start(ap, fmt);
  (void)vdprintf(2, fmt, ap);
  va_end(ap);
}
#endif

#ifndef __libc_enable_secure
# define __libc_enable_secure 0
#endif

/* ================================================================
 * Section: libc-pointer-arith.h
 * ================================================================ */
#ifndef ALIGN_UP
# define ALIGN_UP(x, a)   (((x) + ((a) - 1)) & ~((a) - 1))
#endif
#ifndef ALIGN_DOWN
# define ALIGN_DOWN(x, a) ((x) & ~((a) - 1))
#endif

#ifndef PTR_PLUS
#  define PTR_PLUS(PTR, OFF) ((void *)((char *)(PTR) + (ptrdiff_t)(OFF)))
#endif
#ifndef PTR_DIFF
#  define PTR_DIFF(P1, P2) ((ptrdiff_t)((const char *)(P1) - (const char *)(P2)))
#endif
#ifndef PTR_ALIGN_DOWN
#  define PTR_ALIGN_DOWN(PTR, ALIGN) \
    ((void *)((uintptr_t)(PTR) & ~((uintptr_t)(ALIGN) - 1)))
#endif
#ifndef PTR_ALIGN_UP
#  define PTR_ALIGN_UP(PTR, ALIGN) \
    ((void *)((((uintptr_t)(PTR) + ((uintptr_t)(ALIGN) - 1)) & ~((uintptr_t)(ALIGN) - 1))))
#endif
#ifndef PTR_IS_ALIGNED
#  define PTR_IS_ALIGNED(PTR, ALIGN) \
    ((((uintptr_t)(PTR)) & ((uintptr_t)(ALIGN) - 1)) == 0)
#endif

/* ================================================================
 * Section: not-cancel.h (map to plain syscalls in standalone builds)
 * ================================================================ */
#ifndef __open_nocancel
#  define __open_nocancel(...) open(__VA_ARGS__)
#endif
#ifndef __open64_nocancel
#  define __open64_nocancel(...) open(__VA_ARGS__)
#endif
#ifndef __read_nocancel
#  define __read_nocancel(fd, buf, n) read((fd), (buf), (n))
#endif
#ifndef __close_nocancel
#  define __close_nocancel(fd) close(fd)
#endif
#ifndef __close_nocancel_nostatus
#  define __close_nocancel_nostatus(fd) ((void)close(fd))
#endif

/* ================================================================
 * Section: random-bits.h
 *   - Provide random_bits() -> 32-bit entropy
 *   - Best-effort priority: getrandom (nonblocking) -> /dev/urandom -> time() xorshift
 * ================================================================ */
#ifndef __STANDALONE_HAVE_GETRANDOM_FLAG
# define __STANDALONE_HAVE_GETRANDOM_FLAG 1
#endif
#ifndef GRND_NONBLOCK
# define GRND_NONBLOCK 0x0001
#endif

/* Internal helper that tries to fill a buffer with entropy. */
static inline attribute_unused void __random_bits_fill(void *buf, size_t len)
{
  unsigned char *p = (unsigned char *)buf;
  size_t off = 0;

  /* 1) getrandom (nonblocking) if available */
#if __STANDALONE_HAVE_SYS_RANDOM
  {
    ssize_t r = getrandom(p, len, GRND_NONBLOCK);
    if (r > 0) off = (size_t)r;
  }
#endif

  /* 2) /dev/urandom (best-effort) */
  if (off < len) {
    int fd = __open_nocancel("/dev/urandom", O_RDONLY | O_CLOEXEC);
    if (fd >= 0) {
      while (off < len) {
        ssize_t n = __read_nocancel(fd, p + off, len - off);
        if (n <= 0) break;
        off += (size_t)n;
      }
      __close_nocancel_nostatus(fd);
    }
  }

  /* 3) time()-based xorshift fallback (not cryptographic; sufficient for tcache key) */
  if (off < len) {
    uint64_t seed = (uint64_t)time(NULL) ^ (uint64_t)(uintptr_t)&seed;
    /* xorshift64* */
    for (; off + sizeof(uint64_t) <= len; off += sizeof(uint64_t)) {
      seed ^= seed >> 12;
      seed ^= seed << 25;
      seed ^= seed >> 27;
      uint64_t v = seed * 2685821657736338717ULL;
      *(uint64_t *)(p + off) = v;
    }
    if (off < len) {
      uint64_t v = seed * 2685821657736338717ULL;
      for (; off < len; ++off) p[off] = (unsigned char)(v & 0xFF), v >>= 8;
    }
  }
}

/* Public helper returning 32 random bits as uint32_t. */
#ifndef random_bits
static inline attribute_unused uint32_t __random_bits_u32(void)
{
  uint32_t v = 0;
  __random_bits_fill(&v, sizeof(v));
  return v;
}
# define random_bits() __random_bits_u32()
#endif

/* ================================================================
 * Section: libc-mtag.h (no-op defaults; guarded)
 * ================================================================ */
#ifndef LIBC_MTAG_ENABLED
# define LIBC_MTAG_ENABLED 0
#endif
#ifndef MTAG_ENABLED
# define MTAG_ENABLED 0
#endif
#ifndef __libc_mtag_enabled
# define __libc_mtag_enabled 0
#endif
/* malloc.c often provides its own 'static bool mtag_enabled = false;'.
 * Keep this macro only as a default when not provided elsewhere. */
#ifndef mtag_enabled
# define mtag_enabled false
#endif

#ifndef MTAG_TAG_PTR
# define MTAG_TAG_PTR(p, tag) (p)
#endif
#ifndef MTAG_STRIP_PTR
# define MTAG_STRIP_PTR(p) (p)
#endif
#ifndef MTAG_MEMSET_TAG
# define MTAG_MEMSET_TAG(p, len, tag) do { (void)(p); (void)(len); (void)(tag); } while (0)
#endif
#ifndef MTAG_EXTRACT
# define MTAG_EXTRACT(p) (0)
#endif

/* In your unified shim (e.g., glibc_shim.h) */
#ifndef __LIBC_MTAG_STUBS
#define __LIBC_MTAG_STUBS
#  include <stddef.h>
#  define __libc_mtag_tag_region(p,n)         (p)
#  define __libc_mtag_tag_zero_region(p,n)    (p)
#  define __libc_mtag_new_tag(p)              (p)
#  define __libc_mtag_address_get_tag(p)      (p)
/* If some call site expects an integer tag from address_get_tag, instead use 0:
 *   #define __libc_mtag_address_get_tag(p)  (0)
 * Pick the variant that matches your call-site return type.
 */
#endif

/* ================================================================
 * Section: setvmaname.h (no-op defaults; guarded)
 * ================================================================ */
 /* In compat_next/setvmaname.h or your unified shim, before arena.c is included */
#ifndef __set_vma_name
#  define __set_vma_name(start,len,name) ((void)0)
#endif

/* ================================================================
 * Section: shlib-compat.h (no-op defaults; guarded)
 * ================================================================ */

/* Minimal alias/versioning shim for our allocator build. */

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
#  define strong_alias(sym, alias) \
     extern __typeof__(sym) alias __attribute__((alias(#sym)))
#endif

#ifndef weak_alias
#  define weak_alias(sym, alias) \
     extern __typeof__(sym) alias __attribute__((weak, alias(#sym)))
#endif

/* End of glibc_shim.h */
