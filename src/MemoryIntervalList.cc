#include <system_error>
#include <sys/mman.h> 
#include "MemoryIntervalList.h"
#include "globals.h"

MemoryIntervalList::MemoryIntervalList() :
    _list_capcaity(0),
    _list_length(0) {
    }

void *MemoryIntervalList::AllocateMemory(size_t size) {
    size_t length = ROUND_UP(size, PageSize::BASE_4KB);
    return _mmap(NULL, length, MMAP_PROTECTION, MMAP_FLAGS, -1, 0); 
}

//ASK: here _munmap fail when size = 0; so i just return withuot doing nothig, thats ok?
int MemoryIntervalList::FreeMemory(void* addr, size_t size) {
    if(size == 0) {
        return 0;
    }
    size_t length = ROUND_UP(size, PageSize::BASE_4KB);
    return _munmap(addr, length);
}

void MemoryIntervalList::Initialize(MmapFuncPtr allocator,
        MunmapFuncPtr deallocator,
        size_t capcaity) {
    _list_capcaity = capcaity;
    _list_length = 0;
    _mmap = allocator;
    _munmap = deallocator;
    _interval_list = static_cast<MemoryInterval*>(
            AllocateMemory(capcaity * sizeof (MemoryInterval)));
    if (_interval_list == nullptr) {
        std::error_code ec(errno, std::generic_category());
        THROW_EXCEPTION("Failed to allocate Memory Region Interval List");
    }
}

MemoryIntervalList::~MemoryIntervalList() {
    int res = FreeMemory(_interval_list, _list_capcaity);
    if (res != 0) {
        THROW_EXCEPTION("Failed to deallocate Memory Region Interval List");
    }
}

void MemoryIntervalList::AddInterval(
        off_t start_offset, off_t end_offset,
        PageSize page_size) {
    if (_list_length == _list_capcaity) {
        THROW_EXCEPTION("Memory Region Interval List is already full");
    }
    _interval_list[_list_length]._start_offset = start_offset;
    _interval_list[_list_length]._end_offset = end_offset;
    _interval_list[_list_length]._page_size = page_size;
    _list_length++;
}

void MemoryIntervalList::SwapIntetrvals(int i, int j) {
    // do the swap field by fierld to prevent calling copy-ctor
    auto start_offset = _interval_list[i]._start_offset;
    _interval_list[i]._start_offset = _interval_list[j]._start_offset;
    _interval_list[j]._start_offset = start_offset;

    auto end_offset = _interval_list[i]._end_offset;
    _interval_list[i]._end_offset = _interval_list[j]._end_offset;
    _interval_list[j]._end_offset = end_offset;

    auto page_size = _interval_list[i]._page_size;
    _interval_list[i]._page_size = _interval_list[j]._page_size;
    _interval_list[j]._page_size = page_size;
}

void MemoryIntervalList::Sort() {
    // Min/Max Sort
    for (unsigned int i=0; i<_list_length; i++) {
        for (unsigned int j=i; j<_list_length; j++) {
            if (!MemoryInterval::LessThan(_interval_list[i],
                        _interval_list[j])) {
                SwapIntetrvals(i, j);
            }
        }
    }
}

size_t MemoryIntervalList::GetLength()
{
    return _list_length;
}

MemoryInterval& MemoryIntervalList::At(int i)
{
    return _interval_list[i];
}

MemoryInterval* MemoryIntervalList::FirstIntervalOf(PageSize pageSize)
{
    for (unsigned int i = 0 ; i < _list_length; i++) {
        if (_interval_list[i]._page_size == pageSize)
            return &(_interval_list[i]);
    }
    return nullptr;
}

void MemoryIntervalList::CopyMemoryIntervalsOf1GBTo(MemoryIntervalList &listToFillWith1GBIntervals) {
    listToFillWith1GBIntervals.Initialize(_mmap, _munmap, _list_length);
    for(unsigned int i = 0 ; i < _list_length; i++) {
        off_t start_offset = this->At(i)._start_offset;
        off_t end_offset = this->At(i)._end_offset;
        auto page_size = static_cast<size_t>(this->At(i)._page_size);
        if (page_size == (size_t )PageSize ::HUGE_1GB)
            listToFillWith1GBIntervals.AddInterval(start_offset, end_offset, PageSize::HUGE_1GB);
    }
}

void MemoryIntervalList::CopyMemoryIntervalsOf2MBTo(MemoryIntervalList &listToFillWith2MBIntervals) {
    listToFillWith2MBIntervals.Initialize(_mmap, _munmap, _list_length);
    for(unsigned int i = 0 ; i < _list_length; i++) {
        off_t start_offset = this->At(i)._start_offset;
        off_t end_offset = this->At(i)._end_offset;
        auto page_size = static_cast<size_t>(this->At(i)._page_size);
        if (page_size == (size_t )PageSize ::HUGE_2MB)
            listToFillWith2MBIntervals.AddInterval(start_offset, end_offset, PageSize::HUGE_2MB);
    }
}

off_t MemoryIntervalList::FindMaxEndOffset() {
    off_t max_end_offset = 0;
    for(unsigned int i = 0 ; i < _list_length; i++) {
        off_t current_end_offset = _interval_list[i]._end_offset;
        if (max_end_offset < current_end_offset) {
            max_end_offset = current_end_offset;
        }
    }
    return max_end_offset;
}



