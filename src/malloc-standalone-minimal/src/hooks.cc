// Local copy for malloc-standalone-minimal build
// Removed #include <malloc.h> to avoid conflicts with standalone malloc

#include <stdio.h>
#include <stdlib.h>
// #include <malloc.h>  // Removed - conflicts with standalone malloc
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <dlfcn.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>

#include "hooks.h"
#include "GlibcAllocationFunctions.h"
#include "MemoryAllocator.h"

// Forward declarations for malloc.c symbols
extern "C" {
    extern void *(*__morecore)(ptrdiff_t);
    extern int mallopt(int param, int value);
    // mallopt parameters from our standalone malloc
    #ifndef M_MMAP_MAX
    #define M_MMAP_MAX -4
    #endif
    #ifndef M_TRIM_THRESHOLD
    #define M_TRIM_THRESHOLD -1
    #endif
    #ifndef M_TOP_PAD
    #define M_TOP_PAD -2
    #endif
    #ifndef M_ARENA_MAX
    #define M_ARENA_MAX -8
    #endif
}

static void constructor() __attribute__((constructor));
static void destructor() __attribute__((destructor));
static void setup_morecore();

MemoryAllocator hpbrs_allocator;
void* sys_heap_top = nullptr;
void* brk_top = nullptr;
bool is_library_initialized = false;
std::mutex g_hook_sbrk_mutex;
std::mutex g_hook_brk_mutex;
std::mutex g_hook_mmap_mutex;
//std::mutex g_hook_malloc_mutex;
bool alloc_request_intercepted = false;

//void *(*__morecore)(ptrdiff_t) = sbrk;
//__morecore = sbrk;

/**
 * this flag is used to prevent potential deadlock in some memory allocators 
 * (such as dlmalloc):
 * is_inside_malloc_api will be toggled to true if and only if we are inside
 * some malloc API which is intercepted in this file (i.e., either malloc, 
 * calloc, or free).
 * This flag is used to prevent deadlocks caused by a recursive calls to these 
 * malloc APIs.
 * The recursive calls could be happened (as our observations) only in case
 * that dlsym calls _dlerror_run before their local buffers were initialized
 * and then _dlerror_run will call calloc to allocate the error buffer.
 * We recorded few scenarios where this could happen:
 * 1) malloc* (during loading time) -> dlsym (to call glibc-malloc) -> 
 *    _dlerror_run -> calloc*
 * 2) free* (after we already loaded and initialized) -> deallocate memory from
 *    internal pools -> resize pool (shrink) -> dlsym (to call glibc-munmap)
 *    -> _dlerror_run -> calloc*
 * APIs marked with (*) are the APIs that acquires the malloc API lock.
 *
**/
bool is_inside_malloc_api = false;

/*
 * Mosalloc serves allocation requests by overriding glibc morecore hook.
 * glibc is supposed to call morecore each time an allocation request is sent
 * but glibc has no free slot that can be used to serve this request.
 * After Mosalloc is loaded there could be a few leftover free slots in glibc
 * free list that can be used to serve small allocation requests even that we
 * want to serve them from Mosalloc pools. For preventing such a scenario we
 * will consume all leftover free slots remained in glibc before loading 
 * Mosalloc to force all allocation requests to be served by Mosalloc pools.
 */
void consume_glibc_free_slots() {
    size_t chunk_size = 16;
    while(true) {
        alloc_request_intercepted = false;
        GlibcAllocationFunctions local_glibc_funcs;
        void* ptr = local_glibc_funcs.CallGlibcMalloc(chunk_size);
        if (!ptr) {
            break;
        }
        if (alloc_request_intercepted == true) {
            //local_glibc_funcs.CallGlibcMalloc(4096 - chunk_size - 16);
            break;
        }
    }
}

static void setup_morecore() {
    GlibcAllocationFunctions local_glibc_funcs;
    void* temp_brk_top = local_glibc_funcs.CallGlibcSbrk(0);
    temp_brk_top = (void*)ROUND_UP((size_t)temp_brk_top, PageSize::HUGE_1GB);
    _brk_region_base = temp_brk_top;
    
    // disable mmap by setting M_MMAP_MAX to 0 in order to force glibc to
    // allocate all memory chunks through morecore (to prevent using internal
    // glibc mmap for allocating memory chunks greater than M_MMAP_THRESHOLD
    mallopt(M_MMAP_MAX, 0);

    // disable heap trimming to prevent shrinking the heap below our heap base
    // address (which is an aligned---rounded up---address of the system heap
    // base to the nearest hugepage aligned address. This is done because we
    // are not allowed to violate brk semantics and to prevent heap corruption.
    mallopt(M_TRIM_THRESHOLD, -1);
    
    mallopt(M_TOP_PAD, 0);

    // Limit glibc malloc arenas to 1 arena. This is done to prevent glibc from
    // creating new arenas by calling its internal mmap (and then we cannot
    // intercept it and back it with hugepages). Glibc creates  additional
    // memory allocation arenas if mutex contention is detected (in a
    // multi-threaded applications)
    mallopt(M_ARENA_MAX, 1);
    
    __morecore = mosalloc_morecore;
}

static void activate_mosalloc() {
    is_library_initialized = true;
    setup_morecore();
    consume_glibc_free_slots();
}

static void deactivate_mosalloc() {
    hpbrs_allocator.AnalyzeRegions();
    is_library_initialized = false;
}

extern char *__progname;
static void constructor() {
    if (!strcmp(__progname, "orted")) {
        return;
    }
    //printf("\n%s\n", __progname);
    activate_mosalloc();
}

static void destructor() {
    deactivate_mosalloc();
}

int mprotect(void *addr, size_t len, int prot) __THROW_EXCEPTION {
    if (hpbrs_allocator.IsInitialized() == true &&
        hpbrs_allocator.IsAddressInHugePageRegions(addr) == true) {
        return 0;
    }
    GlibcAllocationFunctions local_glibc_funcs;
    return local_glibc_funcs.CallGlibcMprotect(addr, len, prot);
}

void *mmap(void *addr, size_t length, int prot, int flags, int fd, 
            off_t offset) __THROW_EXCEPTION {
    if (hpbrs_allocator.IsInitialized() == false) {
        GlibcAllocationFunctions local_glibc_funcs;
        return local_glibc_funcs.CallGlibcMmap(addr, length, prot, flags, fd, offset);
    }
    
    MUTEX_GUARD(g_hook_mmap_mutex);

    if (fd >= 0) {
        return hpbrs_allocator.AllocateFromFileMmapRegion(addr, length, prot, flags, fd, offset);
        //GlibcAllocationFunctions local_glibc_funcs;
        //return local_glibc_funcs.CallGlibcMmap(addr, length, prot, flags, fd, offset);
    }
    return hpbrs_allocator.AllocateFromAnonymousMmapRegion(length);
}

int munmap(void *addr, size_t length) __THROW_EXCEPTION {
    if (hpbrs_allocator.IsInitialized() == false) {
        GlibcAllocationFunctions local_glibc_funcs;
        return local_glibc_funcs.CallGlibcMunmap(addr, length);
    }

    MUTEX_GUARD(g_hook_mmap_mutex);

    int res = hpbrs_allocator.DeallocateFromMmapRegion(addr, length);
    return res;
}

int brk(void *addr) __THROW_EXCEPTION {
    if (hpbrs_allocator.IsInitialized() == false) {
        GlibcAllocationFunctions local_glibc_funcs;
        return local_glibc_funcs.CallGlibcBrk(addr);
    }
    
    MUTEX_GUARD(g_hook_brk_mutex);

    return hpbrs_allocator.ChangeProgramBreak(addr);
}

void *mosalloc_morecore(intptr_t increment) __THROW_EXCEPTION {
    alloc_request_intercepted = true;
    void* ptr = sbrk(increment);
    return ptr;
}

void *sbrk(intptr_t increment) __THROW_EXCEPTION {
    if (hpbrs_allocator.IsInitialized() == false) {
        GlibcAllocationFunctions local_glibc_funcs;
        return local_glibc_funcs.CallGlibcSbrk(increment);
    }
    
    MUTEX_GUARD(g_hook_sbrk_mutex);

    // if this the first call to sbrk after pools were initialized
    // then initialize brk_top to be the brk pool base address
    if (brk_top == nullptr) {
        brk_top = hpbrs_allocator.GetBrkRegionBase();
    }

    /*
     * On success, sbrk() returns the previous program break.  (If the break 
     * was increased, then this value is a pointer to the start of the newly 
     * allocated memory).  On error, (void *) -1 is returned, and errno is 
     * set to ENOMEM.
    */
    
    void* prev_brk = brk_top;
    void* new_brk = (void*) ((intptr_t)brk_top + increment);

    if (brk(new_brk) < 0) {
        return ((void*)-1);
    }

    brk_top = new_brk;
    return prev_brk;
}
