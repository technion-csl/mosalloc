// src/morecore.c
#include <stddef.h>     // ptrdiff_t
#include <stdbool.h>    // bool
#include <unistd.h>     // sbrk (via __sbrk mapping)
#include "glibc_compat.h"  // e.g., #define __sbrk sbrk
#include "trace.h"


static bool __always_fail_morecore = false;

void *__glibc_morecore(ptrdiff_t increment) {
    if (__always_fail_morecore) return NULL;
    void *result = (void *)__sbrk(increment);
    if (result == (void *)-1) return NULL;
    return result;
}

#if SHLIB_COMPAT (libc, GLIBC_2_0, GLIBC_2_34)
compat_symbol (libc, __glibc_morecore, __default_morecore, GLIBC_2_0);
#endif