#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <malloc.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include "GlibcAllocationFunctions.h"
#include "globals.h"

#define ASSERT_TRUE(exp) { \
    if (!(exp)) { \
        printf("assert((%s)[=%d])\n", #exp, (exp)); \
        assert(exp); \
    }}

GlibcAllocationFunctions::GlibcAllocationFunctions() {
    _real_calloc = reinterpret_cast<void *(*)(size_t, size_t)>(
            dlsym(RTLD_NEXT, "calloc"));
    ASSERT_TRUE(NULL != _real_calloc);
 
    _real_malloc = reinterpret_cast<void *(*)(size_t)>(
            dlsym(RTLD_NEXT, "malloc"));
    ASSERT_TRUE(NULL != _real_malloc);
    
    _real_realloc = reinterpret_cast<void *(*)(void*, size_t)>(
            dlsym(RTLD_NEXT, "realloc"));
    ASSERT_TRUE(NULL != _real_realloc);

    _real_free = reinterpret_cast<void (*)(void *)>(dlsym(RTLD_NEXT, "free"));
    ASSERT_TRUE(NULL != _real_free);
    
    _real_mmap = reinterpret_cast<void *(*)(
            void *, size_t, int, int, int, off_t)>(dlsym(RTLD_NEXT, "mmap"));
    if (_real_mmap == NULL) {
        printf("error: %s\n", dlerror());
    }
    ASSERT_TRUE(NULL != _real_mmap);

    _real_mprotect = reinterpret_cast<int (*)(
            void *, size_t, int)>(dlsym(RTLD_NEXT, "mprotect"));
    ASSERT_TRUE(NULL != _real_mprotect);

    _real_munmap = reinterpret_cast<int (*)(
            void *, size_t)>(dlsym(RTLD_NEXT, "munmap"));
    ASSERT_TRUE(NULL != _real_munmap);

    _real_brk = reinterpret_cast<int(*)(void*)>(
            dlsym(RTLD_NEXT, "brk"));
    ASSERT_TRUE(NULL != _real_brk);

    _real_sbrk = reinterpret_cast<void* (*)(intptr_t)>(dlsym(RTLD_NEXT, "sbrk"));
    ASSERT_TRUE(NULL != _real_sbrk);
}

void* GlibcAllocationFunctions::CallGlibcCalloc(size_t nmemb, size_t size) {
    return _real_calloc(nmemb, size);
}

void* GlibcAllocationFunctions::CallGlibcMalloc(size_t size) {
    return _real_malloc(size);
}

void* GlibcAllocationFunctions::CallGlibcRealloc(void* ptr, size_t size) {
    return _real_realloc(ptr, size);
}

void GlibcAllocationFunctions::CallGlibcFree(void* ptr) {
    _real_free(ptr);
}

int GlibcAllocationFunctions::CallGlibcMprotect(void *addr,
        size_t length,
        int prot) {
    return _real_mprotect(addr, length, prot);
}

void * GlibcAllocationFunctions::CallGlibcMmap(void *addr,
        size_t length,
        int prot,
        int flags,
        int fd,
        off_t offset) {
    return _real_mmap(addr, length, prot, flags, fd, offset);
}

int GlibcAllocationFunctions::CallGlibcMunmap(void *addr, size_t length) {
    return _real_munmap(addr, length);
}

int GlibcAllocationFunctions::CallGlibcBrk(void* addr) {
    return _real_brk(addr);
}

void* GlibcAllocationFunctions::CallGlibcSbrk(intptr_t increment) {
    return _real_sbrk(increment);
}


