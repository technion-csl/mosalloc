#ifndef _HUGE_PAGE_BACKED_REGIONS_ALLOCATOR_H_
#define _HUGE_PAGE_BACKED_REGIONS_ALLOCATOR_H_

#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include "../include/GlibcAllocationFunctions.h"
#include "../include/HugePageBackedRegion.h"
#include "../include/FirstFitAllocator.h"
#include "../include/HugePagesConfiguration.h"
#include "ParseCsv.h"

#ifdef THREAD_SAFETY
#define MUTEX_GUARD(lock) std::lock_guard<std::mutex> guard(lock) 
#else //THREAD_SAFETY
#define MUTEX_GUARD(lock)
#endif //THREAD_SAFETY

extern void *_brk_region_base;

class MemoryAllocator {
    public:
        MemoryAllocator();
        ~MemoryAllocator();

        void* AllocateFromAnonymousMmapRegion(size_t);
        void* AllocateFromFileMmapRegion(void *, size_t, int, int, int, off_t);
        int DeallocateFromMmapRegion(void*, size_t);
        int ChangeProgramBreak(void *addr);
        void* GetBrkRegionBase();
        bool IsAddressInHugePageRegions(void *addr);
        void AnalyzeRegions();
        
        /*
         * IsInitialized is used to detect when the library is already 
         * initialized.
         * Since we are loading this library using LD_PRELOAD, the loader
         * will add it as the first library in the search path, but it will
         * initialize it last one on the initialization chani, see:
         * https://sourceware.org/bugzilla/show_bug.cgi?id=14379
         * Also, the loader could call malloc APIs during load and before
         * our library was initialized; in this case we are in trouble 
         * because our internal data structures is still not initialized but
         * malloc is already called and we have to serve it. Therefore, we 
         * are using this flag to detect when our library is initialized and
         * only then we can serve malloc APIs from our internal pools, 
         * otherwise these calls will be redirected to glibs using dlsym
         *
        */
        bool IsInitialized() { return _isInitialized; }

    private:
        void InitRegions(void *brk_region_base);
        int DeallocateFromAnonymousMmapRegion(void*, size_t);
        int DeallocateFromFileMmapRegion(void*, size_t);
        void SetIntervalConfigList(PoolConfigurationData &configurationData, const char *config_file,
                                   const char *pool_type);


        bool _isInitialized = false;
        FirstFitAllocator _mmap_anon_ffa;
        FirstFitAllocator _mmap_file_ffa;
        HugePageBackedRegion _mmap_anon_hpbr;
        HugePageBackedRegion _mmap_file_hpbr;
        HugePageBackedRegion _brk_hpbr;
        MemoryIntervalsValidator _intervals_configuration_validator;

        GlibcAllocationFunctions _glibc_funcs;

#ifdef THREAD_SAFETY
        std::mutex _anon_mmap_mutex;
        std::mutex _file_mmap_mutex;
        std::mutex _brk_mutex;
#endif // THREAD_SAFETY

        bool _analyze_hpbrs;
        size_t _anon_mmap_max_size;
        size_t _file_mmap_max_size;
        size_t _brk_max_size;

};

#endif /* _HUGE_PAGE_BACKED_REGIONS_ALLOCATOR_H_ */
