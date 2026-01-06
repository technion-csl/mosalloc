// Local copy for malloc-standalone-minimal build
// Modified to work with standalone minimal malloc instead of glibc
// Uses direct syscalls instead of GlibcAllocationFunctions to avoid conflicts

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
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

// Direct syscall wrappers to avoid recursion through our overridden functions
static inline void* sys_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    return (void*)syscall(SYS_mmap, addr, length, prot, flags, fd, offset);
}

static inline int sys_munmap(void *addr, size_t length) {
    return syscall(SYS_munmap, addr, length);
}

static inline int sys_brk(void *addr) {
    void* result = (void*)syscall(SYS_brk, addr);
    return (result == (void*)-1) ? -1 : 0;
}

static inline void* sys_sbrk(intptr_t increment) {
    void* current = (void*)syscall(SYS_brk, 0);
    if (current == (void*)-1) return (void*)-1;
    
    void* new_brk = (void*)((intptr_t)current + increment);
    void* result = (void*)syscall(SYS_brk, new_brk);
    
    if (result == (void*)-1 || result < new_brk) {
        errno = ENOMEM;
        return (void*)-1;
    }
    return current;
}

static inline int sys_mprotect(void *addr, size_t len, int prot) {
    return syscall(SYS_mprotect, addr, len, prot);
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
bool alloc_request_intercepted = false;
bool is_inside_malloc_api = false;

// For minimal malloc, we don't need to consume glibc free slots
// since we're replacing glibc malloc entirely
static void consume_glibc_free_slots() {
    // No-op for minimal malloc - there are no glibc free slots to consume
}

static void setup_morecore() {
    // Get current brk using direct syscall
    void* temp_brk_top = sys_sbrk(0);
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
    return sys_mprotect(addr, len, prot);
}

void *mmap(void *addr, size_t length, int prot, int flags, int fd, 
            off_t offset) __THROW_EXCEPTION {
    if (hpbrs_allocator.IsInitialized() == false) {
        return sys_mmap(addr, length, prot, flags, fd, offset);
    }
    
    MUTEX_GUARD(g_hook_mmap_mutex);

    if (fd >= 0) {
        return hpbrs_allocator.AllocateFromFileMmapRegion(addr, length, prot, flags, fd, offset);
    }
    return hpbrs_allocator.AllocateFromAnonymousMmapRegion(length);
}

int munmap(void *addr, size_t length) __THROW_EXCEPTION {
    if (hpbrs_allocator.IsInitialized() == false) {
        return sys_munmap(addr, length);
    }

    MUTEX_GUARD(g_hook_mmap_mutex);

    int res = hpbrs_allocator.DeallocateFromMmapRegion(addr, length);
    return res;
}

int brk(void *addr) __THROW_EXCEPTION {
    if (hpbrs_allocator.IsInitialized() == false) {
        return sys_brk(addr);
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
        return sys_sbrk(increment);
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
