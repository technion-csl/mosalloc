//
// Created by a.mohammad on 3/10/2019.
//
// Note: hugepages should be pre-allocated before running this test (at least 2 pages of 1GB and 1024 pages of 2MB
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <cstdlib>

#include <string>
#include <iostream>
#include <cstdio>
#include <array>

#include "HugePageBackedRegion.h"
#include "NumaMaps.h"
#include "globals.h"
#include "gtest/gtest.h"

#define GB (1073741824)
#define MB (1048576)

class HugePageBackedRegionTest : public ::testing::Test {
 public:

  HugePageBackedRegion _hpbr;
  HugePageBackedRegionTest() : _hpbr() {}
  ~HugePageBackedRegionTest() {}

  void SetUp() override {
    // reserve huge pages as the max region size of HugePageBackedRegion tests
    // currently it's set to 2GB, in case further tests will be added with
    // larger regions, then the reserve command should be adapted accordingly
    std::string cmd = "reserveHugePages.sh -n0 -l1024 -h4";
    std::string cwd(get_current_dir_name());
    cmd = cwd + "/" + cmd;
    ASSERT_EQ(system(cmd.c_str()), 0);
  }

  void test_numa_maps(void *start_addr, size_t size, size_t page_size) {
    NumaMaps numa_maps(getpid());
    auto memory_range = numa_maps.GetMemoryRange(start_addr);
    EXPECT_EQ(memory_range._total_size, size);
    EXPECT_EQ(static_cast<size_t>(memory_range._page_size), page_size);
    EXPECT_EQ(memory_range._type, NumaMaps::MemoryRange::Type::ANONYMOUS);
    EXPECT_EQ(memory_range._total_pages, (size / page_size));
  }

   void test_huge_page_backed_region(size_t size,
                                     MemoryIntervalList& configurationList) {
       _hpbr.Initialize(size, configurationList, mmap, munmap);
       void *region_base = _hpbr.GetRegionBase();
       _hpbr.Resize(0);
       _hpbr.Resize(size);
       memset(region_base, -1, size);
       off_t start_current = 0;
       for(unsigned int i = 0; i < configurationList.GetLength(); i++){
           off_t start_huge_page = configurationList.At(i)._start_offset;
           off_t end_huge_page = configurationList.At(i)._end_offset;
           auto page_size = static_cast<size_t>(configurationList.At(i)._page_size);
           if(start_current < start_huge_page) {
               auto size_4kb_window = (size_t) (start_huge_page - start_current);
               test_numa_maps((void *) ((off_t) region_base + start_current),
                              (size_t) size_4kb_window, static_cast<size_t>(PageSize::BASE_4KB));
               start_current = start_huge_page;
           }
           test_numa_maps((void *) ((off_t) region_base + start_current),
                          (size_t) (end_huge_page - start_huge_page), page_size);
           start_current = end_huge_page;
        }
        if (start_current < (off_t) size) {
            test_numa_maps((void *) ((off_t) region_base + start_current),
                           (size_t) (size - start_current), static_cast<size_t>(PageSize::BASE_4KB));
        }
        _hpbr.Resize(0);
   }

  void test_huge_page_backed_region(size_t size,
                                    off_t start_1gb,
                                    off_t end_1gb,
                                    off_t start_2mb,
                                    off_t end_2mb) {
      MemoryIntervalList configurationList;
      configurationList.Initialize(mmap, munmap, 2);
      if(end_1gb - start_1gb > 0) {
          configurationList.AddInterval(start_1gb, end_1gb, PageSize::HUGE_1GB);
      }
      if(end_2mb - start_2mb > 0) {
          configurationList.AddInterval(start_2mb, end_2mb, PageSize::HUGE_2MB);
      }
      test_huge_page_backed_region(size, configurationList);
  }
};

TEST_F(HugePageBackedRegionTest, TestAllocateRegionOfPages_1GB) {
  test_huge_page_backed_region(1 * GB, 0, 1 * GB, 0, 0);
}

TEST_F(HugePageBackedRegionTest, TestAllocateRegionOfPages_1GB_2MB) {
  test_huge_page_backed_region(2ULL * GB, 0, 1 * GB, 1 * GB, 2ULL * GB);
}

TEST_F(HugePageBackedRegionTest, TestAllocateRegionOfPages_2MB) {
  test_huge_page_backed_region(1 * GB, 0, 0, 0, 1 * GB);
}

TEST_F(HugePageBackedRegionTest, TestAllocateRegionOfPages_2MB_1GB) {
  test_huge_page_backed_region(2ULL * GB, 1 * GB, 2ULL * GB, 0, 1 * GB);
}

// This test was removed because it could fail in some systems due to the 
// fact that the kernel may merge adjacent memory areas. We observed that
// in some cases, the loader allocates 3 pages (of total size = 12KB) for 
// TLS/TCB and then the Mosalloc pool will be allocated exactly before this
// memory area, which causes the kernel to merge these two regions to one
// region and then the test fails when asserting the region size
/*
TEST_F(HugePageBackedRegionTest, TestAllocateRegionOfPages_4KB) {
  test_huge_page_backed_region(1 * GB, 0, 0, 0, 0);
}
*/

TEST_F(HugePageBackedRegionTest, TestAllocateRegionOfPages_4KB_1GB) {
  test_huge_page_backed_region(2ULL * GB, 1 * GB, 2ULL * GB, 0, 0);
}

TEST_F(HugePageBackedRegionTest, TestAllocateRegionOfPages_4KB_1GB_4KB) {
  test_huge_page_backed_region(2ULL * GB, 100 * MB, (100 * MB + 1 * GB), 0,0);
}

TEST_F(HugePageBackedRegionTest, TestAllocateRegionOfPages_4KB_1GB_4KB_2MB_4KB) {
  size_t size = 2ULL * GB;
  off_t start_1gb = 100 * MB;
  off_t end_1gb = start_1gb + 1 * GB;
  off_t start_2mb = end_1gb + 100 * MB;
  off_t end_2mb = start_2mb + 100 * MB;
  test_huge_page_backed_region(size, start_1gb, end_1gb, start_2mb, end_2mb);
}

TEST_F(HugePageBackedRegionTest, TestAllocateRegionOfPages_4KB_2MB) {
  test_huge_page_backed_region(1 * GB, 0, 0, 100 * MB, 1 * GB);
}

TEST_F(HugePageBackedRegionTest, TestAllocateRegionOfPages_4KB_2MB_4KB) {
  test_huge_page_backed_region(1 * GB, 0, 0, 101 * MB, 201 * MB);
}

TEST_F(HugePageBackedRegionTest, TestAllocateRegionOfPages_4KB_2MB_4KB_1GB_4KB) {
  size_t size = 2ULL * GB;
  off_t start_2mb = 11 * MB;
  off_t end_2mb = start_2mb + 100 * MB;
  off_t start_1gb = end_2mb + 100 * MB;
  off_t end_1gb = start_1gb + 1 * GB;
    MemoryIntervalList configurationList;
    configurationList.Initialize(mmap, munmap, 5);
    configurationList.AddInterval(start_2mb, end_2mb, PageSize::HUGE_2MB);
    configurationList.AddInterval(start_1gb, end_1gb, PageSize::HUGE_1GB);
    test_huge_page_backed_region(size, configurationList);
}

TEST_F(HugePageBackedRegionTest, TestAllocateMultiplyRegions2MBFirst) {
    size_t size = 5ULL * GB;
    off_t start_2mb_1 = 12 * MB;
    off_t end_2mb_1 = start_2mb_1 + 50 * MB;
    off_t start_1gb_1 = end_2mb_1 + 50 * MB;
    off_t end_1gb_1 = start_1gb_1 + 1 * GB;
    off_t start_2mb_2 = end_1gb_1 + 50 * MB;
    off_t end_2mb_2 = start_2mb_2 + 500 * MB;
    off_t start_1gb_2 = end_2mb_2 + 474 * MB;
    off_t end_1gb_2 = start_1gb_2 + 1 * GB;
    //|4KB - 12MB |2MB - 50 MB |4KB - 50MB | 1GB - 1GB | 4KB - 50 MB| 2MB - 500 MB |4KB - 474MB | 1GB - 1GB | 4KB - 781MB|

    MemoryIntervalList configurationList;
    configurationList.Initialize(mmap, munmap, 4);
    configurationList.AddInterval(start_1gb_1, end_1gb_1, PageSize::HUGE_1GB);
    configurationList.AddInterval(start_1gb_2, end_1gb_2, PageSize::HUGE_1GB);
    configurationList.AddInterval(start_2mb_2, end_2mb_2, PageSize::HUGE_2MB);
    configurationList.AddInterval(start_2mb_1, end_2mb_1, PageSize::HUGE_2MB);
    test_huge_page_backed_region(size, configurationList);
}

TEST_F(HugePageBackedRegionTest, TestMultiplyWindows2MBFirstStartFromZero) {
    size_t size = 5ULL * GB;
    off_t start_2mb_1 = 0;
    off_t end_2mb_1 = start_2mb_1 + 50 * MB;
    off_t start_1gb_1 = end_2mb_1 + 50 * MB;
    off_t end_1gb_1 = start_1gb_1 + 1 * GB;
    off_t start_2mb_2 = end_1gb_1 + 50 * MB;
    off_t end_2mb_2 = start_2mb_2 + 500 * MB;
    off_t start_1gb_2 = end_1gb_1 + 1 * GB;
    off_t end_1gb_2 = start_1gb_2 + 1 * GB;
    off_t start_2mb_3 = end_1gb_2;
    off_t end_2mb_3 = start_2mb_3 + 10 * MB;
    //|2MB - 50 MB |4KB - 50MB | 1GB - 1GB | 4KB - 50 MB| 2MB - 500 MB |4KB - 474MB | 1GB - 1GB | 2MB - 10MB|

    MemoryIntervalList configurationList;
    configurationList.Initialize(mmap, munmap, 5);
    configurationList.AddInterval(start_1gb_1, end_1gb_1, PageSize::HUGE_1GB);
    configurationList.AddInterval(start_1gb_2, end_1gb_2, PageSize::HUGE_1GB);
    configurationList.AddInterval(start_2mb_2, end_2mb_2, PageSize::HUGE_2MB);
    configurationList.AddInterval(start_2mb_1, end_2mb_1, PageSize::HUGE_2MB);
    configurationList.AddInterval(start_2mb_3, end_2mb_3, PageSize::HUGE_2MB);
    test_huge_page_backed_region(size, configurationList);
}

TEST_F(HugePageBackedRegionTest, TestMultiplyWindows1GBFirst) {
    size_t size = 5ULL * GB;
    off_t start_1gb_1 = 12 * MB;
    off_t end_1gb_1 = start_1gb_1 + 1 * GB;
    off_t start_2mb_1 = end_1gb_1 + 12 * MB;
    off_t end_2mb_1 = start_2mb_1 + 50 * MB;
    off_t start_2mb_2 = end_2mb_1 + 50 * MB;
    off_t end_2mb_2 = start_2mb_2 + 50 * MB;
    off_t start_1gb_2 = end_1gb_1 + 1 * GB;
    off_t end_1gb_2 = start_1gb_2 + 1 * GB;
    //|4KB - 12 MB |1GB - 1GB| 4KB - 12MB | 2MB - 50MB | 4KB - 50 MB| 2MB - 50 MB |4KB - 862MB | 1GB - 1GB | 2MB - 10MB|

    MemoryIntervalList configurationList;
    configurationList.Initialize(mmap, munmap, 4);
    configurationList.AddInterval(start_1gb_1, end_1gb_1, PageSize::HUGE_1GB); //1GB
    configurationList.AddInterval(start_1gb_2, end_1gb_2, PageSize::HUGE_1GB); //4GB
    configurationList.AddInterval(start_2mb_1, end_2mb_1, PageSize::HUGE_2MB);
    configurationList.AddInterval(start_2mb_2, end_2mb_2, PageSize::HUGE_2MB);
    test_huge_page_backed_region(size, configurationList);
}

TEST_F(HugePageBackedRegionTest, TestMultiplyWindows1GBFirstStartFromZero) {
    size_t size = 5ULL * GB;
    off_t start_1gb_1 = 0;
    off_t end_1gb_1 = start_1gb_1 + 1 * GB;
    off_t start_2mb_1 = end_1gb_1 + 12 * MB;
    off_t end_2mb_1 = start_2mb_1 + 50 * MB;
    off_t start_2mb_2 = end_2mb_1 + 50 * MB;
    off_t end_2mb_2 = start_2mb_2 + 50 * MB;
    off_t start_1gb_2 = end_1gb_1 + 1 * GB;
    off_t end_1gb_2 = start_1gb_2 + 1 * GB;
    off_t start_2mb_3 = end_1gb_2;
    off_t end_2mb_3 = start_2mb_3 + 50 * MB;
    //|1GB - 1GB| 4KB - 12MB | 2MB - 50MB | 4KB - 50MB | 2MB - 50MB | 4KB - ***MB|
    //                                          1GB - 1GB | 2MB - 50 MB |4KB - ***MB | 1GB - 1GB |

    MemoryIntervalList configurationList;
    configurationList.Initialize(mmap, munmap, 6);
    configurationList.AddInterval(start_1gb_1, end_1gb_1, PageSize::HUGE_1GB);
    configurationList.AddInterval(start_1gb_2, end_1gb_2, PageSize::HUGE_1GB);
    configurationList.AddInterval(start_2mb_1, end_2mb_1, PageSize::HUGE_2MB);
    configurationList.AddInterval(start_2mb_2, end_2mb_2, PageSize::HUGE_2MB);
    configurationList.AddInterval(start_2mb_3, end_2mb_3, PageSize::HUGE_2MB);
    test_huge_page_backed_region(size, configurationList);
}

TEST_F(HugePageBackedRegionTest, TestMultiplyWindowsWithout2MB) {
    size_t size = 5ULL * GB;
    off_t start_1gb_1 = 0;
    off_t end_1gb_1 = start_1gb_1 + 1 * GB;
    auto start_1gb_2 = (1UL * (end_1gb_1 + 2UL * GB));
    auto end_1gb_2 = start_1gb_2 + 1 * GB;
    //|1GB - 1GB| 4KB - ***MB | 1GB - 2GB |

    MemoryIntervalList configurationList;
    configurationList.Initialize(mmap, munmap, 2);
    configurationList.AddInterval(start_1gb_1, end_1gb_1, PageSize::HUGE_1GB);
    configurationList.AddInterval(start_1gb_2, end_1gb_2, PageSize::HUGE_1GB);
    test_huge_page_backed_region(size, configurationList);
}

TEST_F(HugePageBackedRegionTest, TestMultiplyWindowsOnly1GB) {
    size_t size = 5ULL * GB;
    off_t start_1gb_1 = 0;
    off_t end_1gb_1 = start_1gb_1 + 1 * GB;
    off_t start_1gb_2 = end_1gb_1;
    off_t end_1gb_2 = start_1gb_2 + 1 * GB;
    off_t start_1gb_3 = end_1gb_2;
    off_t end_1gb_3 = start_1gb_3 + 1 * GB;
    //|1GB - 1GB| 1GB - 1GB | 1GB - 1GB |

    MemoryIntervalList configurationList;
    configurationList.Initialize(mmap, munmap, 3);
    configurationList.AddInterval(start_1gb_1, end_1gb_1, PageSize::HUGE_1GB);
    configurationList.AddInterval(start_1gb_2, end_1gb_2, PageSize::HUGE_1GB);
    configurationList.AddInterval(start_1gb_3, end_1gb_3, PageSize::HUGE_1GB);
    test_huge_page_backed_region(size, configurationList);
}

TEST_F(HugePageBackedRegionTest, TestMultiplyWindowsWithout1GB) {
    size_t size = 1ULL * GB;
    off_t start_2mb_1 = 24 * MB;
    off_t end_2mb_1 = start_2mb_1 + 50 * MB;
    off_t start_2mb_2 = end_2mb_1 + 24 * MB;
    off_t end_2mb_2 = start_2mb_2 + 50 * MB;
    off_t start_2mb_3 = end_2mb_2 + 24 * MB;
    off_t end_2mb_3 = start_2mb_3 + 50 * MB;
    //|4KB - 24MB |2MB - 50 MB |4KB - 24MB | 2MB - 50 MB |4KB - 24MB | 2MB - 50 MB | 4KB - ***MB|

    MemoryIntervalList configurationList;
    configurationList.Initialize(mmap, munmap, 5);
    configurationList.AddInterval(start_2mb_1, end_2mb_1, PageSize::HUGE_2MB);
    configurationList.AddInterval(start_2mb_2, end_2mb_2, PageSize::HUGE_2MB);
    configurationList.AddInterval(start_2mb_3, end_2mb_3, PageSize::HUGE_2MB);
    test_huge_page_backed_region(size, configurationList);
}

TEST_F(HugePageBackedRegionTest, TestMultiplyWindowsOnly2M) {
    size_t size = 1ULL * GB;
    off_t start_2mb_1 = 0;
    off_t end_2mb_1 = start_2mb_1 + 500 * MB;
    off_t start_2mb_2 = end_2mb_1;
    off_t end_2mb_2 = start_2mb_2 + 24 * MB;
    off_t start_2mb_3 = end_2mb_2;
    off_t end_2mb_3 = start_2mb_3 + 500 * MB;
    //|2MB - 500 MB | 2MB - 24 MB | 2MB - 500 MB |

    MemoryIntervalList configurationList;
    configurationList.Initialize(mmap, munmap, 5);
    configurationList.AddInterval(start_2mb_1, end_2mb_1, PageSize::HUGE_2MB);
    configurationList.AddInterval(start_2mb_2, end_2mb_2, PageSize::HUGE_2MB);
    configurationList.AddInterval(start_2mb_3, end_2mb_3, PageSize::HUGE_2MB);
    test_huge_page_backed_region(size, configurationList);
}

// This test was removed because it could fail in some systems due to the 
// fact that the kernel may merge adjacent memory areas. We observed that
// in some cases, the loader allocates 3 pages (of total size = 12KB) for 
// TLS/TCB and then the Mosalloc pool will be allocated exactly before this
// memory area, which causes the kernel to merge these two regions to one
// region and then the test fails when asserting the region size
/*
TEST_F(HugePageBackedRegionTest, TestMultiplyWindowsOnly4KB) {
    size_t size = 2ULL * GB;
    MemoryIntervalList configurationList;
    configurationList.Initialize(mmap, munmap, 0);
    test_huge_page_backed_region(size, configurationList);
}
*/

TEST_F(HugePageBackedRegionTest, TestMultiplyWindowsWithout4KB) {
    size_t size = 2ULL * GB;
    off_t start_2mb_1 = 0;
    off_t end_2mb_1 = start_2mb_1 + 500 * MB;
    off_t start_1gb_1 = end_2mb_1;
    auto end_1gb_1 = start_1gb_1 + 1 * GB;
    off_t start_2mb_2 = end_1gb_1;
    off_t end_2mb_2 = start_2mb_2 + 524 * MB;
    //|2MB - 500 MB | 1GB - 2 GB | 2MB - 524 MB |

    MemoryIntervalList configurationList;
    configurationList.Initialize(mmap, munmap, 5);
    configurationList.AddInterval(start_2mb_1, end_2mb_1, PageSize::HUGE_2MB);
    configurationList.AddInterval(start_1gb_1, end_1gb_1, PageSize::HUGE_1GB);
    configurationList.AddInterval(start_2mb_2, end_2mb_2, PageSize::HUGE_2MB);
    test_huge_page_backed_region(size, configurationList);
}

#define WRITTEN_DATA (0x34)
using namespace std;
void ValidateData(char *ptr, size_t size) {
  for (unsigned int i=0; i<size; i += (MB)) {
    ASSERT_EQ(ptr[i], WRITTEN_DATA)
        << "Written data was overwritten: written-data: " << WRITTEN_DATA
        << " , new-data: " << ptr[i];
  }
}

TEST_F(HugePageBackedRegionTest, TestAllocateRegionOfPages_4KB_2MB_Unaligned) {
    size_t size = 10*MB;
    off_t start_2mb = 3*MB;
    off_t end_2mb = 5*MB;

    MemoryIntervalList configurationList;
    configurationList.Initialize(mmap, munmap, 1);
    configurationList.AddInterval(start_2mb, end_2mb, PageSize::HUGE_2MB);

    _hpbr.Initialize(size, configurationList, mmap, munmap);
    void *region_base = _hpbr.GetRegionBase();

    for (unsigned int i=0; i<=size; i += (MB)) {
        _hpbr.Resize(i);
        memset(region_base, WRITTEN_DATA, i);
        ValidateData((char*)region_base, i);
    }

    for (unsigned int i=0; i<=size; i += (MB)) {
        _hpbr.Resize(size-i);
        ValidateData((char*)region_base, size-i);
    }
    _hpbr.Resize(0);
}

TEST_F(HugePageBackedRegionTest, TestAllocateRegionOfPages_1GB_AndResize) {
    size_t size = 1*GB;
    MemoryIntervalList configurationList;
    configurationList.Initialize(mmap, munmap, 1);
    configurationList.AddInterval(0, size, PageSize::HUGE_1GB);

    _hpbr.Initialize(size, configurationList, mmap, munmap);
    void *region_base = _hpbr.GetRegionBase();
    _hpbr.Resize(0);
    _hpbr.Resize(size);
    memset(region_base, WRITTEN_DATA, size);

    ValidateData((char*)region_base, size);

    for (unsigned int i=size; i>0; i -= (2*MB)) {
        _hpbr.Resize(i);
        ValidateData((char*)region_base, size);
    }
    _hpbr.Resize(0);
}
