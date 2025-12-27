#ifndef _HUGE_PAGE_BACKED_REGION_H
#define _HUGE_PAGE_BACKED_REGION_H

#include <cstddef>
#include <sys/types.h>
#include <vector>
#include "../include/globals.h"
#include "../include/MemoryIntervalList.h"

class HugePageBackedRegion {
    public:

    void Initialize(size_t region_size,
                    MemoryIntervalList& intervalList,
                    MmapFuncPtr allocator,
                    MunmapFuncPtr deallocator,
                    void* region_base = nullptr);

        HugePageBackedRegion();
        ~HugePageBackedRegion();

        int Resize(size_t new_size);

        void *GetRegionBase();

        size_t GetRegionSize();
        
        size_t GetRegionMaxSize();

    private:
        size_t ExtendRegion(size_t new_size);

        size_t ShrinkRegion(size_t new_size);

        void *AllocateMemory(void *start_address, size_t len, PageSize page_size);

        void DeallocateMemory(void *addr, size_t len);

        void* RegionIntervalListMemAlloc(size_t s);

        int RegionIntervalListMemDealloc(void* addr, size_t s);

        void *_region_start;
        size_t _region_max_size;
        MemoryIntervalList _region_intervals;
        size_t _region_current_size;
        bool _initialized;

        MmapFuncPtr _memory_allocator;
        MunmapFuncPtr _memory_deallocator;
};


#endif //_HUGE_PAGE_BACKED_REGION_H
