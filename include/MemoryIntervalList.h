#ifndef _MEMORY_INTERVAL_LIST_H_
#define _MEMORY_INTERVAL_LIST_H_

#include <sys/types.h>
#include "globals.h"

class MemoryInterval {
    public:
        MemoryInterval() {}
        ~MemoryInterval() {}

        off_t _start_offset;
        off_t _end_offset;
        PageSize _page_size;

        static bool LessThan(const MemoryInterval &lhs,
                const MemoryInterval &rhs) {
            return lhs._start_offset < rhs._start_offset;
        }
};

typedef void* (*MmapFuncPtr)(void *, size_t, int, int, int, off_t);
typedef int (*MunmapFuncPtr)(void *, size_t);

class MemoryIntervalList {
    public:
        MemoryIntervalList();
        void Initialize(MmapFuncPtr allocator,
                MunmapFuncPtr deallocator,
                size_t capcaity);
        ~MemoryIntervalList();

        size_t GetLength();
        MemoryInterval& At(int i);

        void AddInterval(off_t start_offset, off_t end_offset, PageSize page_size);
        void Sort();
        MemoryInterval* FirstIntervalOf(PageSize pageSize);
        void CopyMemoryIntervalsOf1GBTo(MemoryIntervalList &listToFillWith1GBIntervals);
        void CopyMemoryIntervalsOf2MBTo(MemoryIntervalList &listToFillWith2MBIntervals);
        off_t FindMaxEndOffset();

    private:
        void SwapIntetrvals(int i, int j);
        void* AllocateMemory(size_t size);
        int FreeMemory(void* addr, size_t size);

        MemoryInterval* _interval_list;
        size_t _list_capcaity;
        size_t _list_length;
        MmapFuncPtr _mmap;
        MunmapFuncPtr _munmap;
};

#endif //_MEMORY_INTERVAL_LIST_H_

