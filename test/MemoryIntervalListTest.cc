#include <iostream>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>

#include "gtest/gtest.h"
#include "MemoryIntervalList.h"



TEST(MemoryIntervalListTest, MixedIntervals) {
    MemoryIntervalList l;
    l.Initialize(mmap, munmap, 10);
    l.AddInterval(1<<20, 1<<30, PageSize::HUGE_2MB);
    l.AddInterval(1<<10, 1<<20, PageSize::BASE_4KB);
    l.AddInterval(0, 1<<10, PageSize::HUGE_1GB);

    EXPECT_EQ(l.At(0)._start_offset, (1<<20));
    EXPECT_EQ(l.At(1)._start_offset, (1<<10));
    EXPECT_EQ(l.At(2)._start_offset, (0));

    l.Sort();

    EXPECT_EQ(l.At(2)._start_offset, (1<<20));
    EXPECT_EQ(l.At(1)._start_offset, (1<<10));
    EXPECT_EQ(l.At(0)._start_offset, (0));

    MemoryIntervalList ls;
    ls.Initialize(mmap, munmap, 20);
    ls.AddInterval(1ul<<30, 1ul<<32, PageSize::HUGE_1GB);
    ls.AddInterval(0, 1<<14, PageSize::BASE_4KB);
    ls.AddInterval(1ul<<38, 1ul<<40, PageSize::HUGE_1GB);
    ls.AddInterval(1ul<<32, 1ul<<34, PageSize::HUGE_1GB);
    ls.AddInterval(1ul<<34, 1ul<<36, PageSize::HUGE_2MB);
    ls.AddInterval(1<<24, 1ul<<30, PageSize::HUGE_2MB);
    ls.AddInterval(1<<14, 1<<19, PageSize::BASE_4KB);
    ls.AddInterval(1<<20, 1<<24, PageSize::HUGE_2MB);
    ls.AddInterval(1ul<<36, 1ul<<38, PageSize::HUGE_1GB);
    ls.AddInterval(1<<19, 1<<20, PageSize::BASE_4KB);


    EXPECT_EQ(ls.At(0)._start_offset, (1ul<<30));
    EXPECT_EQ(ls.At(1)._start_offset, (0));
    EXPECT_EQ(ls.At(2)._start_offset, (1ul<<38));
    EXPECT_EQ(ls.At(3)._start_offset, (1ul<<32));
    EXPECT_EQ(ls.At(4)._start_offset, (1ul<<34));
    EXPECT_EQ(ls.At(5)._start_offset, (1<<24));
    EXPECT_EQ(ls.At(6)._start_offset, (1<<14));
    EXPECT_EQ(ls.At(7)._start_offset, (1<<20));
    EXPECT_EQ(ls.At(8)._start_offset, (1ul<<36));
    EXPECT_EQ(ls.At(9)._start_offset, (1<<19));

    ls.Sort();

    EXPECT_EQ(ls.At(0)._start_offset, (0));
    EXPECT_EQ(ls.At(1)._start_offset, (1<<14));
    EXPECT_EQ(ls.At(2)._start_offset, (1<<19));
    EXPECT_EQ(ls.At(3)._start_offset, (1<<20));
    EXPECT_EQ(ls.At(4)._start_offset, (1<<24));
    EXPECT_EQ(ls.At(5)._start_offset, (1ul<<30));
    EXPECT_EQ(ls.At(6)._start_offset, (1ul<<32));
    EXPECT_EQ(ls.At(7)._start_offset, (1ul<<34));
    EXPECT_EQ(ls.At(8)._start_offset, (1ul<<36));
    EXPECT_EQ(ls.At(9)._start_offset, (1ul<<38));
}
