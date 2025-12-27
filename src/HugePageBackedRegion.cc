#include <stdexcept>
#include <sys/mman.h>
#include <system_error>
#include <algorithm>
#include <assert.h>
#include <functional> // fot std::bind

#include "HugePageBackedRegion.h"


void* HugePageBackedRegion::RegionIntervalListMemAlloc(size_t s) {
    assert(_memory_allocator != nullptr);
    size_t length = ROUND_UP(s, PageSize::BASE_4KB);
    return _memory_allocator(NULL, length, MMAP_PROTECTION, MMAP_FLAGS, -1, 0);
}

int HugePageBackedRegion::RegionIntervalListMemDealloc(void* addr, size_t s) {
    assert(_memory_deallocator != nullptr);
    size_t length = ROUND_UP(s, PageSize::BASE_4KB);
    return _memory_deallocator(addr, length);
}

void *HugePageBackedRegion::AllocateMemory(void *start_address,
                                           size_t len,
                                           PageSize page_size) {
    if (len == 0) {
        return start_address;
    }
    int mmap_flags = MMAP_FLAGS;
    if (start_address != nullptr) {
        mmap_flags |= MAP_FIXED;
    }
    if (page_size == PageSize::HUGE_1GB) {
        mmap_flags |= MAP_HUGETLB | MAP_HUGE_1GB;
    } else if (page_size == PageSize::HUGE_2MB) {
        mmap_flags |= MAP_HUGETLB | MAP_HUGE_2MB;
    }
    void *ptr = _memory_allocator(start_address, len, MMAP_PROTECTION, mmap_flags, -1, 0);
    if (ptr == MAP_FAILED) {
        std::error_code ec(errno, std::generic_category());
        THROW_EXCEPTION("failed to allocate memory by mmap");
    }

    return ptr;
}

void HugePageBackedRegion::DeallocateMemory(void *addr, size_t len) {
    if (len == 0) {
        return;
    }

    if (_memory_deallocator(addr, len) != 0) {
        std::error_code ec(errno, std::generic_category());
        THROW_EXCEPTION("failed to unmap allocated memory by munmap");
    }
}

size_t HugePageBackedRegion::ExtendRegion(size_t new_size) {
    size_t updated_region_size = _region_current_size;
    size_t intervals_length = _region_intervals.GetLength();
    for (unsigned int i=0; i<intervals_length; i++) {
        MemoryInterval& interval = _region_intervals.At(i);
        // check if current interval should be extended
        if ((_region_current_size >= (size_t) interval._start_offset
             || new_size >= (size_t) interval._start_offset)
            && _region_current_size < (size_t) interval._end_offset) {
            // Calculate start and end offsets of current required allocation
            off_t start_offset = interval._start_offset;
            off_t end_offset = interval._end_offset;
            // check if current interval is the same interval as
            // _region_current_size in
            if (_region_current_size >= (size_t) interval._start_offset) {
                start_offset = (off_t) _region_current_size;
            }
            // check if the required new_size overlaps current interval
            if (new_size <= (size_t) interval._end_offset) {
                size_t page_size = static_cast<size_t>(interval._page_size);
                size_t sub_interval_size = new_size - interval._start_offset;
                sub_interval_size = ROUND_UP(sub_interval_size, page_size);
                end_offset = interval._start_offset + sub_interval_size;
            } else {
                end_offset = interval._end_offset;
            }
            AllocateMemory((void *) ((size_t) _region_start + start_offset),
                           end_offset - start_offset,
                           interval._page_size);
            updated_region_size = (size_t) end_offset;
        }
    }
    return updated_region_size;
}

size_t HugePageBackedRegion::ShrinkRegion(size_t new_size) {
    size_t updated_region_size = _region_current_size;
    size_t intervals_length = _region_intervals.GetLength();
    for (unsigned int i=0; i<intervals_length; i++) {
        MemoryInterval& interval = _region_intervals.At(i);
        // check if current interval should be extended
        if ((_region_current_size <= (size_t) interval._end_offset
             || new_size <= (size_t) interval._end_offset)
            && _region_current_size > (size_t) interval._start_offset) {
            // Calculate start and end offsets of current required de-allocation
            off_t start_offset = interval._start_offset;
            off_t end_offset = interval._end_offset;
            // check if current interval is the same interval as
            // _region_current_size in
            if (_region_current_size <= (size_t) interval._end_offset) {
                end_offset = (off_t) _region_current_size;
            }
            // check if the required new_size overlaps current interval
            if (new_size >= (size_t) interval._start_offset) {
                size_t page_size = static_cast<size_t>(interval._page_size);
                //start_offset = ROUND_UP(new_size, page_size);
                size_t sub_interval_size = new_size - interval._start_offset;
                sub_interval_size = ROUND_UP(sub_interval_size, page_size);
                start_offset = interval._start_offset + sub_interval_size;
            } else {
                start_offset = interval._start_offset;
            }
            DeallocateMemory((void *) ((size_t) _region_start + start_offset),
                             end_offset - start_offset);
            if (start_offset < (off_t) updated_region_size) {
                updated_region_size = (size_t) start_offset;
            }
        }
    }
    return updated_region_size;
}

HugePageBackedRegion::HugePageBackedRegion() : _initialized(false) {}

void HugePageBackedRegion::Initialize(size_t region_size,
                                      MemoryIntervalList& intervalList,
                                      MmapFuncPtr allocator,
                                      MunmapFuncPtr deallocator,
                                      void* region_base) {
    _region_start = nullptr;
    _region_max_size = region_size;
    _region_current_size = 0;
    _memory_allocator = allocator;
    _memory_deallocator = deallocator;

    size_t max_size = (2 * intervalList.GetLength()) + 1; //In worst case there will be 4KB area between each interval
    _region_intervals.Initialize(allocator, deallocator, max_size);

    _initialized = true;
    
    // Validate region_size is aligned to 4KB pages
    if (!IS_ALIGNED(region_size, PageSize::BASE_4KB)) {
        THROW_EXCEPTION("region size is not aligned to 4KB");
    }

    intervalList.Sort();
    auto first_region_1gb = intervalList.FirstIntervalOf(PageSize::HUGE_1GB);
    auto first_region_2mb = intervalList.FirstIntervalOf(PageSize::HUGE_2MB);

    if (first_region_1gb != nullptr) {
        // Round up the required region size to 1GB and add additional 1GB
        // to align the allocated memory with 1GB addresses.
        _region_current_size = ROUND_UP(region_size + (size_t)PageSize::HUGE_1GB,
                                        PageSize::HUGE_1GB);
    }
    else if (first_region_2mb != nullptr) {
        // Round up the required region size to 2MB and add additional 2MB
        // to align the allocated memory with 2MB addresses.
        _region_current_size = ROUND_UP(region_size + (size_t)PageSize::HUGE_2MB,
                                        PageSize::HUGE_2MB);
    }
    else {
        // Do not round up or align the address since mmap will return an 
        // aligned address to base page size (4KB)
        _region_current_size = region_size;
    }

    // Allocate the rounded-up size with 4KB pages
    void *base_addr = AllocateMemory(region_base, _region_current_size,
                                     PageSize::BASE_4KB);

    // Update _region_start to be aligned with largest page size
    if (first_region_1gb != nullptr) {
        auto start_1gb_offset = first_region_1gb->_start_offset;
        auto start_1gb_ptr = ROUND_UP(((off_t) base_addr +  start_1gb_offset),
                                      PageSize::HUGE_1GB);
        _region_start = (void *) (start_1gb_ptr - start_1gb_offset);
    }
    else if (first_region_2mb != nullptr) {
        auto start_2mb_offset = first_region_2mb->_start_offset;
        auto start_2mb_ptr = ROUND_UP(((off_t) base_addr + start_2mb_offset),
                                      PageSize::HUGE_2MB);
        _region_start = (void *) (start_2mb_ptr - start_2mb_offset);
    }
    else {
        _region_start = base_addr;
    }

    // Update intervals list
    // Add 1GB interval
    for (unsigned int i = 0; i < intervalList.GetLength(); i++) {
        auto interval = intervalList.At(i);
        _region_intervals.AddInterval(interval._start_offset,
                                      interval._end_offset,
                                      interval._page_size);
    }

    _region_intervals.Sort();
    // Add 4KB intervals
    off_t prev_start_offset = 0;
    size_t intervals_length = _region_intervals.GetLength();
    for (unsigned int i = 0; i < intervals_length; i++) {
        MemoryInterval& interval = _region_intervals.At(i);
        if (prev_start_offset < interval._start_offset) {
            _region_intervals.AddInterval(prev_start_offset,
                                  interval._start_offset,
                                  PageSize::BASE_4KB);
        }
        prev_start_offset = interval._end_offset;
    }
    if ((size_t) prev_start_offset < _region_max_size) {
        _region_intervals.AddInterval(prev_start_offset,
                              _region_max_size,
                              PageSize::BASE_4KB);
    }
    _region_intervals.Sort();

    // unmap region memory
    DeallocateMemory(base_addr, _region_current_size);
    _region_current_size = 0;

    //Reallocate region with exact pages sizes (call Resize(size))
    Resize(_region_max_size);
    _region_max_size = _region_current_size;

}

HugePageBackedRegion::~HugePageBackedRegion() {
    // Deallocating memory in the d'tor will cause a seg-fault in exit
    // because loader (dl-fini.c:210) sorts the libraries in exit again
    // according to their dependencies and unloads them, and when 
    // huge_page_backed_region.so library is unloaded the memory will be
    // deallocated and all libraries that were loaded using our memory/regions
    // and appear in loadersorted list after huge_page_backed_region.so then 
    // will seg-fault.

    //DeallocateMemory(_region_start, _region_current_size);
    //_region_max_size = _region_current_size = 0;
    //_region_intervals.clear();
}

int HugePageBackedRegion::Resize(size_t new_size) {
    assert(_initialized);

    if (new_size > _region_max_size) {
        return -ENOMEM;
    }

    if (new_size > _region_current_size) {
        _region_current_size = ExtendRegion(new_size);
    }
    else if (new_size < _region_current_size) {
        _region_current_size = ShrinkRegion(new_size);
    }
    return 0;
}

void *HugePageBackedRegion::GetRegionBase() {
    assert(_initialized);
    return _region_start;
}

size_t HugePageBackedRegion::GetRegionSize() {
    assert(_initialized);
    return _region_current_size;
}

size_t HugePageBackedRegion::GetRegionMaxSize() {
    assert(_initialized);
    return _region_max_size;
}
