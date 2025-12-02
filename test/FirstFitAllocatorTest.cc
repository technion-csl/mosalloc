//
// Created by a.mohammad on 29/9/2019.
//

#include "FirstFitAllocator.h"
#include "globals.h"
#include "gtest/gtest.h"

#define TEST_REGION_START ((void *) (1ul << 30)) // 1GB
#define TEST_REGION_END ((void *) (2ul << 30)) // 2GB
#define TEST_REGION_ALLOCATIONS (256)
#define TEST_REGION_ALLOCATION_SIZE  (((size_t) (PTR_SUB(TEST_REGION_END, TEST_REGION_START)) / TEST_REGION_ALLOCATIONS))


void TestAllocateFreeAllocate(
		void *const start,
		void *const end,
		const size_t base_region_size,
		const unsigned int len,
		const short increasing_size,
		const short reverse_first_alloc,
		const short reverse_free,
		const short reverse_second_alloc,
		const short even_free) {

	FirstFitAllocator _ffa(true, false);
	void **ptrs = (void **) malloc(len * sizeof(void *));
	size_t total_space = (size_t) (PTR_SUB(end, start));
	size_t total_alloc = 0;
	size_t mrl_len = (total_space / base_region_size) * 10;

	_ffa.Initialize(mrl_len, start, end);

	EXPECT_EQ(_ffa.GetFreeSpace(), total_space)
		<< "FFA free space: " << _ffa.GetFreeSpace()
		<< " != Total space: " << total_space;

	// Allocate all memory
	for (unsigned int i = 0; i < len; i++) {

		int index = i;
		if (reverse_first_alloc) {
			index = len - i - 1;
		}
		size_t region_size = base_region_size;
		if (increasing_size) {
			region_size = (index + 1) * base_region_size;
		}

		void *region_start = _ffa.Allocate(region_size);
		EXPECT_EQ(region_start, PTR_ADD(start, total_alloc));
		ptrs[index] = region_start;
		total_alloc += region_size;
		EXPECT_EQ(_ffa.GetFreeSpace(), total_space - total_alloc);
	}

	// Free all memory
	for (unsigned int i = 0; i < len; i++) {

		if (even_free && ((i % 2) == 0)) {
			continue;
		}
		int index = i;
		if (reverse_free) {
			index = len - i - 1;
		}
		size_t region_size = base_region_size;
		if (increasing_size) {
			region_size = (index + 1) * base_region_size;
		}

		int res = _ffa.Free(ptrs[index], region_size);
		EXPECT_EQ(res, 0);
		ptrs[index] = nullptr;
		total_alloc -= region_size;
		EXPECT_EQ(_ffa.GetFreeSpace(), total_space - total_alloc);
	}

	// Reallocate all memory
	for (unsigned int i = 0; i < len; i++) {

		if (even_free && ((i % 2) == 0)) {
			continue;
		}

		int index = i;
		if (reverse_second_alloc) {
			index = len - i - 1;
		}
		size_t region_size = base_region_size;
		if (increasing_size) {
			region_size = (index + 1) * base_region_size;
		}

		if (ptrs[index] != nullptr) { //i.e., it was not freed
			continue;
		}

		void *region_start = _ffa.Allocate(region_size);
		//ASSERT_PTR_EQ(region_start, start + total_alloc);
		ASSERT_NE(region_start, nullptr) << "FFA allocate returned nullptr";
		ptrs[index] = region_start;
		total_alloc += region_size;
		EXPECT_EQ(_ffa.GetFreeSpace(), total_space - total_alloc);

	}
}

void TestAllocateFreeSingleAllocateAllMemory(
		void *const start,
		void *const end,
		const unsigned int len,
		const unsigned short reverse_free) {

	size_t total_space = (size_t) (PTR_SUB(end, start));
	size_t region_size = total_space / len;
	size_t total_alloc = 0;

	FirstFitAllocator _ffa(true, false);
	_ffa.Initialize(len, start, end);

	EXPECT_EQ(_ffa.GetFreeSpace(), total_space);

	// allocate all memory with multiple chunks
	for (unsigned int i = 0; i < len; i++) {
		void *region_start = _ffa.Allocate(region_size);
		EXPECT_EQ(region_start, PTR_ADD(start, total_alloc));
		total_alloc += region_size;
		EXPECT_EQ(_ffa.GetFreeSpace(), total_space - total_alloc);
	}

	// free all memory
	for (unsigned int i = 0; i < len; i++) {
		total_alloc -= region_size;

		void *ptr = PTR_ADD(start, i * region_size);
		if (reverse_free) {
			ptr = PTR_ADD(start, total_alloc);
		}

		int res = _ffa.Free(ptr, region_size);
		EXPECT_EQ(res, 0);
		EXPECT_EQ(_ffa.GetFreeSpace(), total_space - total_alloc);
	}

	EXPECT_EQ(_ffa.GetFreeSpace(), total_space);

	// alloc all memory with one chunk
	void *region_start = _ffa.Allocate(total_space);
	ASSERT_NE(region_start, nullptr);
}

TEST(FirstFitAllocatorTest, AllocateFreeAllocateOneSlice) {
	TestAllocateFreeAllocate(
			TEST_REGION_START,
			TEST_REGION_END,
			TEST_REGION_ALLOCATION_SIZE,
			1, 0, 0, 0, 0, 0);
}

TEST(FirstFitAllocatorTest, AllocateFreeAllocateTwoSlices) {
	TestAllocateFreeAllocate(
			TEST_REGION_START,
			TEST_REGION_END,
			TEST_REGION_ALLOCATION_SIZE,
			2, 0, 0, 0, 0, 0);
}

TEST(FirstFitAllocatorTest, AllocateFreeAllocateAllMemory) {
	TestAllocateFreeAllocate(
			TEST_REGION_START,
			TEST_REGION_END,
			TEST_REGION_ALLOCATION_SIZE,
			TEST_REGION_ALLOCATIONS, 0, 0, 0, 0, 0);
}

TEST(FirstFitAllocatorTest, AllocateFreeEvenAllocate) {
	TestAllocateFreeAllocate(
			TEST_REGION_START,
			TEST_REGION_END,
			TEST_REGION_ALLOCATION_SIZE,
			TEST_REGION_ALLOCATIONS,
			0, 0, 0, 0, 1);
}

TEST(FirstFitAllocatorTest, AllocateFreeEvenReversedAllocate) {
	TestAllocateFreeAllocate(
			TEST_REGION_START,
			TEST_REGION_END,
			TEST_REGION_ALLOCATION_SIZE,
			TEST_REGION_ALLOCATIONS,
			0, 0, 1, 0, 1);
}

TEST(FirstFitAllocatorTest, AllocateReversedFreeEvenAllocate) {
	TestAllocateFreeAllocate(
			TEST_REGION_START,
			TEST_REGION_END,
			TEST_REGION_ALLOCATION_SIZE,
			TEST_REGION_ALLOCATIONS,
			0, 0, 0, 1, 1);
}

TEST(FirstFitAllocatorTest, AllocateReversedFreeEvenReversedAllocate) {
	TestAllocateFreeAllocate(
			TEST_REGION_START,
			TEST_REGION_END,
			TEST_REGION_ALLOCATION_SIZE,
			TEST_REGION_ALLOCATIONS,
			0, 0, 1, 1, 1);
}

TEST(FirstFitAllocatorTest, AllocateReversedIncreasinglyFreeEvenReversedAllocate) {
	// The following test is to cover the broken-free-list bug:
	// the bug is about breaking free list and lossing the last
	// node in free-list which contains the largest free memory
	// region, and that occured when first and one of last allocated
	// chunks were freed, then the free-list will be broken.
	size_t total_space = (size_t) (PTR_SUB(TEST_REGION_END, TEST_REGION_START));
	TestAllocateFreeAllocate(
			TEST_REGION_START,
			TEST_REGION_END,
			(total_space / TEST_REGION_ALLOCATIONS) / TEST_REGION_ALLOCATIONS,
			TEST_REGION_ALLOCATIONS,
			1, 0, 1, 1, 1);
}

TEST(FirstFitAllocatorTest, AllocateFreeSingleAllocateAllMemory) {
	TestAllocateFreeSingleAllocateAllMemory(
			TEST_REGION_START, TEST_REGION_END, TEST_REGION_ALLOCATIONS, 0);
}

TEST(FirstFitAllocatorTest, AllocateFreeSingleReversedAllocateAllMemory) {
	TestAllocateFreeSingleAllocateAllMemory(
			TEST_REGION_START, TEST_REGION_END, TEST_REGION_ALLOCATIONS, 1);
}

TEST(FirstFitAllocatorTest, AllocateIncreasingChunks) {
	TestAllocateFreeAllocate(
			TEST_REGION_START,
			TEST_REGION_END,
			TEST_REGION_ALLOCATION_SIZE / TEST_REGION_ALLOCATIONS,
			TEST_REGION_ALLOCATIONS,
			1, 0, 0, 0, 0);
}

TEST(FirstFitAllocatorTest, AllocateFreeReversedAllocateOneSlice) {
	TestAllocateFreeAllocate(
			TEST_REGION_START,
			TEST_REGION_END,
			TEST_REGION_ALLOCATION_SIZE,
			1, 0, 0, 1, 0, 0);
}

TEST(FirstFitAllocatorTest, AllocateFreeReversedAllocateTwoSlices) {
	TestAllocateFreeAllocate(
			TEST_REGION_START,
			TEST_REGION_END,
			TEST_REGION_ALLOCATION_SIZE,
			2, 0, 0, 1, 0, 0);
}

TEST(FirstFitAllocatorTest, AllocateFreeReversedAllocateAllMemory) {
	TestAllocateFreeAllocate(
			TEST_REGION_START,
			TEST_REGION_END,
			TEST_REGION_ALLOCATION_SIZE,
			TEST_REGION_ALLOCATIONS, 0, 1, 0, 0, 0);
}

TEST(FirstFitAllocatorTest, MemoryIsAvailableButNoNodes) {
	FirstFitAllocator ffa(true, false);
	void *const start = (void *) (1ul << 30); // 1GB
	void *const end = (void *) (2ul << 30); // 2GB
	const unsigned int len = 256;
	size_t region_size = ((size_t) (PTR_SUB(end, start)) / (2 * len));
	size_t total_space = (size_t) (PTR_SUB(end, start));
	size_t total_alloc = 0;

	//len+1: the extra 1 is for the free_head)
	ffa.Initialize(len + 1, start, end);

	EXPECT_EQ(ffa.GetFreeSpace(), total_space);

	// Allocate all nodes
	for (unsigned int i = 0; i < len; i++) {
		void *region_start = ffa.Allocate(region_size);
		EXPECT_EQ(region_start, PTR_ADD(start, total_alloc));
		total_alloc += region_size;
		EXPECT_EQ(ffa.GetFreeSpace(), total_space - total_alloc);
	}

	// free all memory
	for (unsigned int i = 0; i < len; i++) {
		total_alloc -= region_size;
		int res = ffa.Free(PTR_ADD(start, total_alloc), region_size);
		EXPECT_EQ(res, 0);
		EXPECT_EQ(ffa.GetFreeSpace(), total_space - total_alloc);
	}

	EXPECT_EQ(ffa.GetFreeSpace(), total_space);

	// Reallocate all nodes
	for (unsigned int i = 0; i < len; i++) {
		void *region_start = ffa.Allocate(region_size);
		EXPECT_EQ(region_start, PTR_ADD(start, total_alloc));
		total_alloc += region_size;
		EXPECT_EQ(ffa.GetFreeSpace(), total_space - total_alloc);
	}

	ASSERT_NE(ffa.GetFreeSpace(), 0);

	void *region_start = ffa.Allocate(1);
	ASSERT_EQ(region_start, nullptr);
}

TEST(FirstFitAllocatorTest, FreeMiddleChunkAndReallocate) {
	FirstFitAllocator ffa(true, false);
	void *const start = (void *) (1ul << 30); // 1GB
	void *const end = (void *) (2ul << 30); // 2GB
    const unsigned int len = 256;
    size_t total_space = (size_t) (PTR_SUB(end, start));
    size_t total_alloc = 0;
    size_t region_size = total_space / len;
	ffa.Initialize(len, start, end);

	for (unsigned int i = 0; i < len; i++) {
		void *region_start = ffa.Allocate(region_size);
		EXPECT_EQ(region_start, PTR_ADD(start, i * region_size));
		total_alloc += region_size;
		EXPECT_EQ(ffa.GetFreeSpace(), total_space - total_alloc);
	}
	for (unsigned int i = 0; i < len; i++) {
		int status = ffa.Free(PTR_ADD(start, i * region_size), region_size);
		EXPECT_EQ(status, 0);
		total_alloc -= region_size;
		EXPECT_EQ(ffa.GetFreeSpace(), total_space - total_alloc);

		void *region_start = ffa.Allocate(region_size);
		EXPECT_EQ(region_start, PTR_ADD(start, i * region_size));
		total_alloc += region_size;
		EXPECT_EQ(ffa.GetFreeSpace(), total_space - total_alloc);
	}
}

TEST(FirstFitAllocatorTest, AllocateFreeTopHalfAllocateAllMemory) {
	FirstFitAllocator ffa(true, false);
	const unsigned int len = 256;
	void *const start = (void *) (1ul << 30); // 1GB
	void *const end = (void *) (2ul << 30); // 2GB
	size_t total_space = (size_t) (PTR_SUB(end, start));
	size_t total_alloc = 0;

	ffa.Initialize(len, start, end);

	size_t region_size = total_space / len;
	void *region_start = nullptr;

	// Allocate all memory
	for (unsigned int i = 0; i < len; i++) {
		region_start = ffa.Allocate(region_size);
		EXPECT_EQ(region_start, PTR_ADD(start, total_alloc));
		total_alloc += region_size;
		EXPECT_EQ(ffa.GetFreeSpace(), (total_space - total_alloc));
	}

	// Free top half
	for (unsigned int i = len / 2; i < len; i++) {
		int status = ffa.Free(
				PTR_ADD(start, i * region_size),
				region_size);
		EXPECT_EQ(status, 0);
		total_alloc -= region_size;
		EXPECT_EQ(ffa.GetFreeSpace(), (total_space - total_alloc));
	}

	// Reallocate top half
	for (unsigned int i = len / 2; i < len; i++) {
		region_start = ffa.Allocate(region_size);
		ASSERT_NE(region_start, nullptr);
		total_alloc += region_size;
		EXPECT_EQ(ffa.GetFreeSpace(), (total_space - total_alloc));
	}
}
