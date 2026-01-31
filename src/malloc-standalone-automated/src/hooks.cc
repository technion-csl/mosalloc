
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <dlfcn.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>

#include "hooks.h"
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
volatile  bool alloc_request_intercepted = false;
bool is_inside_malloc_api = false;

// Verify that mosalloc_morecore is being called by our malloc
// Keep allocating until morecore is triggered
static void consume_glibc_free_slots() {
    size_t chunk_size = 16;
    while(true) {
        alloc_request_intercepted = false;
        GlibcAllocationFunctions local_glibc_funcs;
        void* ptr = local_glibc_funcs.CallGlibcMalloc(chunk_size);
        if (!ptr) {
            break;
        }
        if (alloc_request_intercepted == true) {
            free(ptr);
            break;
        }
        // Don't free - we want to exhaust pre-existing memory to force morecore call
        chunk_size *= 2;  // Double size to speed up exhaustion
    }
}

static void setup_morecore() {
    
    GlibcAllocationFunctions local_glibc_funcs;
    void* temp_brk_top = local_glibc_funcs.CallGlibcSbrk(0);
    temp_brk_top = (void*)ROUND_UP((size_t)temp_brk_top, PageSize::HUGE_1GB);
    _brk_region_base = temp_brk_top;
        
    // Configure our minimal malloc
    mallopt(M_MMAP_MAX, 0);
    mallopt(M_TRIM_THRESHOLD, -1);
    mallopt(M_TOP_PAD, 0);
    mallopt(M_ARENA_MAX, 1);
    
    __morecore = mosalloc_morecore;
}

static void activate_mosalloc() {
    setup_morecore();
    consume_glibc_free_slots();
    is_library_initialized = true;
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
    activate_mosalloc();
}

static void destructor() {
    deactivate_mosalloc();
}

int mprotect(void *addr, size_t len, int prot) __THROW_EXCEPTION {
    if (is_library_initialized == true && hpbrs_allocator.IsInitialized() == true &&
        hpbrs_allocator.IsAddressInHugePageRegions(addr) == true) {
        return 0;
    }
    GlibcAllocationFunctions local_glibc_funcs;
    return local_glibc_funcs.CallGlibcMprotect(addr, len, prot);
}

void *mmap(void *addr, size_t length, int prot, int flags, int fd, 
            off_t offset) __THROW_EXCEPTION {
    if (is_library_initialized == false || hpbrs_allocator.IsInitialized() == false) {
        GlibcAllocationFunctions local_glibc_funcs;
        return local_glibc_funcs.CallGlibcMmap(addr, length, prot, flags, fd, offset);
    }
    
    MUTEX_GUARD(g_hook_mmap_mutex);

    if (fd >= 0) {
        return hpbrs_allocator.AllocateFromFileMmapRegion(addr, length, prot, flags, fd, offset);
    }
    return hpbrs_allocator.AllocateFromAnonymousMmapRegion(length);
}

int munmap(void *addr, size_t length) __THROW_EXCEPTION {
    if (is_library_initialized == false || hpbrs_allocator.IsInitialized() == false) {
        GlibcAllocationFunctions local_glibc_funcs;
        return local_glibc_funcs.CallGlibcMunmap(addr, length);
    }

    MUTEX_GUARD(g_hook_mmap_mutex);

    int res = hpbrs_allocator.DeallocateFromMmapRegion(addr, length);
    return res;
}

int brk(void *addr) __THROW_EXCEPTION {
    if (is_library_initialized == false || hpbrs_allocator.IsInitialized() == false) {
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
    if (is_library_initialized == false || hpbrs_allocator.IsInitialized() == false) {
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
