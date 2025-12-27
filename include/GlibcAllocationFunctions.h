#ifndef _GLIBC_ALLOCATION_FUNCTIONS_H_
#define _GLIBC_ALLOCATION_FUNCTIONS_H_

#include <cstdio>
#include <unistd.h>

class GlibcAllocationFunctions {
    public:
        GlibcAllocationFunctions();
        ~GlibcAllocationFunctions() {}
        void* CallGlibcCalloc(size_t, size_t);
        void* CallGlibcMalloc(size_t);
        void* CallGlibcRealloc(void*, size_t);
        void CallGlibcFree(void*);
        int CallGlibcMprotect(void *addr, size_t len, int prot);             
        void* CallGlibcMmap(void *, size_t, int, int,int, off_t);
        int CallGlibcMunmap(void *, size_t);
        int CallGlibcBrk(void*);
        void* CallGlibcSbrk(intptr_t);

    private:
        int (*_real_mprotect)(void *, size_t, int);
        void* (*_real_mmap)(void *, size_t, int, int, int, off_t);
        int (*_real_munmap)(void *, size_t);
        void* (*_real_calloc)(size_t, size_t);
        void* (*_real_malloc)(size_t);
        void* (*_real_realloc)(void*, size_t);
        void (*_real_free)(void*);
        int (*_real_brk)(void *addr);
        void* (*_real_sbrk)(intptr_t increment);
};

#endif //_GLIBC_ALLOCATION_FUNCTIONS_H_

