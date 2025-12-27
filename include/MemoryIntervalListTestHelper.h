//
// Created by User on 22/11/2019.
//

#ifndef MOSALLOC_MEMORYINTERVALLISTTESTHELPER_H
#define MOSALLOC_MEMORYINTERVALLISTTESTHELPER_H

#include "MemoryIntervalList.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include "globals.h"
#include <cstdlib>
#include "NumaMaps.h"

#define KB (1024)
#define GB (1073741824)
#define MB (1048576)

class MemoryIntervalListTestHelper {
public:
    static void setMultiplyRegions2MBFirstValidIntervalList(MemoryIntervalList& intervalList) {
        auto start_2mb_1 = 12 * MB;
        auto end_2mb_1 = start_2mb_1 + 50 * MB;
        auto start_1gb_1 = end_2mb_1 + 50 * MB;
        auto end_1gb_1 = start_1gb_1 + 1 * GB;
        auto start_2mb_2 = end_1gb_1 + 50 * MB;
        auto end_2mb_2 = start_2mb_2 + 500 * MB;
        auto start_1gb_2 = end_2mb_2 + 474 * MB;
        auto end_1gb_2 = start_1gb_2 + 1 * GB;
        //|4KB - 12MB |2MB - 50 MB |4KB - 50MB | 1GB - 1GB | 4KB - 50 MB| 2MB - 500 MB |4KB - 474MB | 1GB - 1GB | 4KB - 781MB|

        intervalList.Initialize(mmap, munmap, 4);
        intervalList.AddInterval(start_1gb_1, end_1gb_1, PageSize::HUGE_1GB);
        intervalList.AddInterval(start_1gb_2, end_1gb_2, PageSize::HUGE_1GB);
        intervalList.AddInterval(start_2mb_2, end_2mb_2, PageSize::HUGE_2MB);
        intervalList.AddInterval(start_2mb_1, end_2mb_1, PageSize::HUGE_2MB);
    }

    static void setMultiplyWindows2MBFirstStartFromZeroValidIntervalList(MemoryIntervalList& intervalList) {
        auto start_2mb_1 = 0;
        auto end_2mb_1 = start_2mb_1 + 50 * MB;
        auto start_1gb_1 = end_2mb_1 + 50 * MB;
        auto end_1gb_1 = start_1gb_1 + 1 * GB;
        auto start_2mb_2 = end_1gb_1 + 50 * MB;
        auto end_2mb_2 = start_2mb_2 + 500 * MB;
        auto start_1gb_2 = end_1gb_1 + 1 * GB;
        auto end_1gb_2 = start_1gb_2 + 1 * GB;
        auto start_2mb_3 = end_1gb_2;
        auto end_2mb_3 = start_2mb_3 + 10 * MB;
        //|2MB - 50 MB |4KB - 50MB | 1GB - 1GB | 4KB - 50 MB| 2MB - 500 MB |4KB - 474MB | 1GB - 1GB | 2MB - 10MB|

        intervalList.Initialize(mmap, munmap, 5);
        intervalList.AddInterval(start_1gb_1, end_1gb_1, PageSize::HUGE_1GB);
        intervalList.AddInterval(start_1gb_2, end_1gb_2, PageSize::HUGE_1GB);
        intervalList.AddInterval(start_2mb_2, end_2mb_2, PageSize::HUGE_2MB);
        intervalList.AddInterval(start_2mb_1, end_2mb_1, PageSize::HUGE_2MB);
        intervalList.AddInterval(start_2mb_3, end_2mb_3, PageSize::HUGE_2MB);
    }


    static void setMultiplyWindows1GBFirstValidIntervalList(MemoryIntervalList& intervalList) {
        auto start_1gb_1 = 12 * MB;
        auto end_1gb_1 = start_1gb_1 + 1 * GB;
        auto start_2mb_1 = end_1gb_1 + 12 * MB;
        auto end_2mb_1 = start_2mb_1 + 50 * MB;
        auto start_2mb_2 = end_2mb_1 + 50 * MB;
        auto end_2mb_2 = start_2mb_2 + 50 * MB;
        auto start_1gb_2 = end_1gb_1 + 1 * GB;
        auto end_1gb_2 = start_1gb_2 + 1 * GB;
        //|4KB - 12 MB |1GB - 1GB| 4KB - 12MB | 2MB - 50MB | 4KB - 50 MB| 2MB - 50 MB |4KB - 862MB | 1GB - 1GB | 2MB - 10MB|

        intervalList.Initialize(mmap, munmap, 4);
        intervalList.AddInterval(start_1gb_1, end_1gb_1, PageSize::HUGE_1GB); //1GB
        intervalList.AddInterval(start_1gb_2, end_1gb_2, PageSize::HUGE_1GB); //4GB
        intervalList.AddInterval(start_2mb_1, end_2mb_1, PageSize::HUGE_2MB);
        intervalList.AddInterval(start_2mb_2, end_2mb_2, PageSize::HUGE_2MB);
    }

    static void setMultiplyWindows1GBFirstStartFromZeroValidIntervalList(MemoryIntervalList& intervalList) {
        auto start_1gb_1 = 0;
        auto end_1gb_1 = start_1gb_1 + 1 * GB;
        auto start_2mb_1 = end_1gb_1 + 12 * MB;
        auto end_2mb_1 = start_2mb_1 + 50 * MB;
        auto start_2mb_2 = end_2mb_1 + 50 * MB;
        auto end_2mb_2 = start_2mb_2 + 50 * MB;
        auto start_1gb_2 = end_1gb_1 + 1 * GB;
        auto end_1gb_2 = start_1gb_2 + 1 * GB;
        auto start_2mb_3 = end_1gb_2;
        auto end_2mb_3 = start_2mb_3 + 50 * MB;
        //|1GB - 1GB| 4KB - 12MB | 2MB - 50MB | 4KB - 50MB | 2MB - 50MB | 4KB - ***MB|
        //                                          1GB - 1GB | 2MB - 50 MB |4KB - ***MB | 1GB - 1GB |

        intervalList.Initialize(mmap, munmap, 6);
        intervalList.AddInterval(start_1gb_1, end_1gb_1, PageSize::HUGE_1GB);
        intervalList.AddInterval(start_1gb_2, end_1gb_2, PageSize::HUGE_1GB);
        intervalList.AddInterval(start_2mb_1, end_2mb_1, PageSize::HUGE_2MB);
        intervalList.AddInterval(start_2mb_2, end_2mb_2, PageSize::HUGE_2MB);
        intervalList.AddInterval(start_2mb_3, end_2mb_3, PageSize::HUGE_2MB);
    }

    static void setMultiplyWindowsWithout2MBValidIntervalList(MemoryIntervalList& intervalList) {
        auto start_1gb_1 = 0;
        auto end_1gb_1 = start_1gb_1 + 1 * GB;
        auto start_1gb_2 = (1UL * (end_1gb_1 + 2UL * GB));
        auto end_1gb_2 = start_1gb_2 + 1 * GB;
        //|1GB - 1GB| 4KB - ***MB | 1GB - 2GB |

        intervalList.Initialize(mmap, munmap, 2);
        intervalList.AddInterval(start_1gb_1, end_1gb_1, PageSize::HUGE_1GB);
        intervalList.AddInterval(start_1gb_2, end_1gb_2, PageSize::HUGE_1GB);
    }

    static void setMultiplyWindowsOnly1GValidIntervalList(MemoryIntervalList& intervalList) {
        unsigned long start_1gb_1 = 0;
        unsigned long end_1gb_1 = start_1gb_1 + 1 * GB;
        unsigned long start_1gb_2 = end_1gb_1;
        unsigned long end_1gb_2 = start_1gb_2 + 1 * GB;
        unsigned long start_1gb_3 = end_1gb_2;
        unsigned long end_1gb_3 = start_1gb_3 + 1 * GB;
        //|1GB - 1GB| 1GB - 1GB | 1GB - 1GB |

        intervalList.Initialize(mmap, munmap, 3);
        intervalList.AddInterval(start_1gb_1, end_1gb_1, PageSize::HUGE_1GB);
        intervalList.AddInterval(start_1gb_2, end_1gb_2, PageSize::HUGE_1GB);
        intervalList.AddInterval(start_1gb_3, end_1gb_3, PageSize::HUGE_1GB);
    }

    static void setMultiplyWindowsWithout1GBValidIntervalList(MemoryIntervalList& intervalList) {
        auto start_2mb_1 = 24 * MB;
        auto end_2mb_1 = start_2mb_1 + 50 * MB;
        auto start_2mb_2 = end_2mb_1 + 24 * MB;
        auto end_2mb_2 = start_2mb_2 + 50 * MB;
        auto start_2mb_3 = end_2mb_2 + 24 * MB;
        auto end_2mb_3 = start_2mb_3 + 50 * MB;
        //|4KB - 24MB |2MB - 50 MB |4KB - 24MB | 2MB - 50 MB |4KB - 24MB | 2MB - 50 MB | 4KB - ***MB|

        intervalList.Initialize(mmap, munmap, 5);
        intervalList.AddInterval(start_2mb_1, end_2mb_1, PageSize::HUGE_2MB);
        intervalList.AddInterval(start_2mb_2, end_2mb_2, PageSize::HUGE_2MB);
        intervalList.AddInterval(start_2mb_3, end_2mb_3, PageSize::HUGE_2MB);
    }

    static void setMultiplyWindowsOnly2MBValidIntervalList(MemoryIntervalList& intervalList) {
        auto start_2mb_1 = 0;
        auto end_2mb_1 = start_2mb_1 + 500 * MB;
        auto start_2mb_2 = end_2mb_1;
        auto end_2mb_2 = start_2mb_2 + 24 * MB;
        auto start_2mb_3 = end_2mb_2;
        auto end_2mb_3 = start_2mb_3 + 500 * MB;
        //|2MB - 500 MB | 2MB - 24 MB | 2MB - 500 MB |

        intervalList.Initialize(mmap, munmap, 5);
        intervalList.AddInterval(start_2mb_1, end_2mb_1, PageSize::HUGE_2MB);
        intervalList.AddInterval(start_2mb_2, end_2mb_2, PageSize::HUGE_2MB);
        intervalList.AddInterval(start_2mb_3, end_2mb_3, PageSize::HUGE_2MB);
    }

    static void setEmptyIntervalList(MemoryIntervalList& intervalList) {
        intervalList.Initialize(mmap, munmap, 0);
    }

    static void setListInvalidOffsetBetweenTwo1GBIntervalsError(MemoryIntervalList& intervalList) {
        auto start_1gb_1 = 12 * MB;
        auto end_1gb_1 = start_1gb_1 + 1 * GB;
        auto start_2mb_1 = end_1gb_1 + 12 * MB;
        auto end_2mb_1 = start_2mb_1 + 50 * MB;
        auto start_2mb_2 = end_2mb_1 + 50 * MB;
        auto end_2mb_2 = start_2mb_2 + 50 * MB;
        auto start_1gb_2 = end_1gb_1 + 1 * GB + 12 * MB;
        auto end_1gb_2 = start_1gb_2 + 1 * GB;

        intervalList.Initialize(mmap, munmap, 4);
        intervalList.AddInterval(start_1gb_1, end_1gb_1, PageSize::HUGE_1GB);
        intervalList.AddInterval(start_1gb_2, end_1gb_2, PageSize::HUGE_1GB);
        intervalList.AddInterval(start_2mb_1, end_2mb_1, PageSize::HUGE_2MB);
        intervalList.AddInterval(start_2mb_2, end_2mb_2, PageSize::HUGE_2MB);
    }

    static void setListInvalidSizeOf1GBIntervalError(MemoryIntervalList& intervalList) {
        auto start_1gb_1 = 12 * MB;
        auto end_1gb_1 = start_1gb_1 + 1 * GB;
        auto start_2mb_1 = end_1gb_1 + 12 * MB;
        auto end_2mb_1 = start_2mb_1 + 50 * MB;
        auto start_2mb_2 = end_2mb_1 + 50 * MB;
        auto end_2mb_2 = start_2mb_2 + 50 * MB;
        auto start_1gb_2 = end_1gb_1 + 1 * GB;
        auto end_1gb_2 = start_1gb_2 + 1 * GB - 12 * MB;

        intervalList.Initialize(mmap, munmap, 4);
        intervalList.AddInterval(start_1gb_1, end_1gb_1, PageSize::HUGE_1GB);
        intervalList.AddInterval(start_1gb_2, end_1gb_2, PageSize::HUGE_1GB);
        intervalList.AddInterval(start_2mb_1, end_2mb_1, PageSize::HUGE_2MB);
        intervalList.AddInterval(start_2mb_2, end_2mb_2, PageSize::HUGE_2MB);
    }

    static void setListInvalidOffsetBetweenTwo2MBIntervalsError(MemoryIntervalList& intervalList) {
        auto start_1gb_1 = 12 * MB;
        auto end_1gb_1 = start_1gb_1 + 1 * GB;
        auto start_2mb_1 = end_1gb_1 + 12 * MB;
        auto end_2mb_1 = start_2mb_1 + 50 * MB;
        auto start_2mb_2 = end_2mb_1 + 50 * MB - 1 * MB;
        auto end_2mb_2 = start_2mb_2 + 50 * MB;
        auto start_1gb_2 = end_1gb_1 + 1 * GB;
        auto end_1gb_2 = start_1gb_2 + 1 * GB;

        intervalList.Initialize(mmap, munmap, 4);
        intervalList.AddInterval(start_1gb_1, end_1gb_1, PageSize::HUGE_1GB);
        intervalList.AddInterval(start_1gb_2, end_1gb_2, PageSize::HUGE_1GB);
        intervalList.AddInterval(start_2mb_1, end_2mb_1, PageSize::HUGE_2MB);
        intervalList.AddInterval(start_2mb_2, end_2mb_2, PageSize::HUGE_2MB);
    }

    static void setListInvalidSizeOf2MBIntervalError(MemoryIntervalList& intervalList) {
        auto start_1gb_1 = 12 * MB;
        auto end_1gb_1 = start_1gb_1 + 1 * GB;
        auto start_2mb_1 = end_1gb_1 + 12 * MB;
        auto end_2mb_1 = start_2mb_1 + 50 * MB;
        auto start_2mb_2 = end_2mb_1 + 50 * MB;
        auto end_2mb_2 = start_2mb_2 + 50 * MB - 1 * MB;
        auto start_1gb_2 = end_1gb_1 + 1 * GB;
        auto end_1gb_2 = start_1gb_2 + 1 * GB;

        intervalList.Initialize(mmap, munmap, 4);
        intervalList.AddInterval(start_1gb_1, end_1gb_1, PageSize::HUGE_1GB);
        intervalList.AddInterval(start_1gb_2, end_1gb_2, PageSize::HUGE_1GB);
        intervalList.AddInterval(start_2mb_1, end_2mb_1, PageSize::HUGE_2MB);
        intervalList.AddInterval(start_2mb_2, end_2mb_2, PageSize::HUGE_2MB);
    }

    static void setListInvalidOffsetBetween1GBAnd2MBIntervalsError(MemoryIntervalList& intervalList) {
        auto start_1gb_1 = 12 * MB;
        auto end_1gb_1 = start_1gb_1 + 1 * GB;
        auto start_2mb_1 = end_1gb_1 + 12 * MB - 1 * MB;
        auto end_2mb_1 = start_2mb_1 + 50 * MB;
        auto start_2mb_2 = end_2mb_1 + 50 * MB;
        auto end_2mb_2 = start_2mb_2 + 50 * MB;
        auto start_1gb_2 = end_1gb_1 + 1 * GB;
        auto end_1gb_2 = start_1gb_2 + 1 * GB;

        intervalList.Initialize(mmap, munmap, 4);
        intervalList.AddInterval(start_1gb_1, end_1gb_1, PageSize::HUGE_1GB);
        intervalList.AddInterval(start_1gb_2, end_1gb_2, PageSize::HUGE_1GB);
        intervalList.AddInterval(start_2mb_1, end_2mb_1, PageSize::HUGE_2MB);
        intervalList.AddInterval(start_2mb_2, end_2mb_2, PageSize::HUGE_2MB);
    }

    static void setListInvalidPageSizeError(MemoryIntervalList& intervalList) {
        auto start_2mb_1 = 0;
        auto end_2mb_1 = start_2mb_1 + 500 * MB;
        auto start_2mb_2 = end_2mb_1;
        auto end_2mb_2 = start_2mb_2 + 24 * MB;

        intervalList.Initialize(mmap, munmap, 2);
        intervalList.AddInterval(start_2mb_1, end_2mb_1, PageSize::BASE_4KB);
        intervalList.AddInterval(start_2mb_2, end_2mb_2, PageSize::HUGE_2MB);
    }
};


#endif //MOSALLOC_MEMORYINTERVALLISTTESTHELPER_H
