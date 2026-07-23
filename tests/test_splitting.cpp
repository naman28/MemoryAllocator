/// @file test_splitting.cpp
/// @brief Milestone 3: Block-splitting tests.

#include "test_framework.h"
#include "allocator.h"

using namespace mem;

// ---- Basic split ----

TEST(Split_BasicSplit) {
    HeapAllocator alloc(4096);
    // Initial block payload = 4096 − 64 = 4032.
    void* p1 = alloc.allocate(64);
    ASSERT_NOT_NULL(p1);

    auto stats = alloc.getStats();
    // After split: [USED 64] [FREE remainder]
    ASSERT_EQ(stats.allocated_bytes, size_t(64));
    ASSERT_EQ(stats.total_blocks, size_t(2));
    ASSERT_EQ(stats.free_blocks, size_t(1));
}

// ---- Multiple splits ----

TEST(Split_MultipleSplits) {
    HeapAllocator alloc(4096);
    void* p1 = alloc.allocate(64);
    void* p2 = alloc.allocate(128);
    void* p3 = alloc.allocate(64);
    ASSERT_NOT_NULL(p1);
    ASSERT_NOT_NULL(p2);
    ASSERT_NOT_NULL(p3);

    auto stats = alloc.getStats();
    ASSERT_EQ(stats.allocated_bytes, size_t(64 + 128 + 64));
    ASSERT_EQ(stats.total_blocks, size_t(4));  // 3 used + 1 free
    ASSERT_EQ(stats.free_blocks, size_t(1));
}

// ---- No split when remainder too small ----

TEST(Split_NoSplitWhenTooSmall) {
    HeapAllocator alloc(256);
    // Payload = 256 − 64 = 192.
    // Request 128: remaining = 192 − 128 = 64 < sizeof(BlockHeader) + MIN = 128.
    // → No split; the block keeps its full 192 B payload.
    void* p = alloc.allocate(128);
    ASSERT_NOT_NULL(p);

    auto stats = alloc.getStats();
    ASSERT_EQ(stats.total_blocks, size_t(1));
    ASSERT_EQ(stats.free_blocks, size_t(0));
    ASSERT_EQ(stats.allocated_bytes, size_t(192));  // Entire block, no split.
}

// ---- Verify remainder size ----

TEST(Split_VerifyRemainderSize) {
    HeapAllocator alloc(4096);
    void* p = alloc.allocate(64);
    ASSERT_NOT_NULL(p);

    auto stats = alloc.getStats();
    // Initial payload: 4032.
    // After allocating 64: remaining = 4032 − 64 = 3968.
    // New free block: 3968 − 64 (header) = 3904 payload.
    ASSERT_EQ(stats.free_bytes, size_t(3904));
    ASSERT_EQ(stats.allocated_bytes, size_t(64));
    alloc.deallocate(p);
}

// ---- Exact fit — no split ----

TEST(Split_ExactFit) {
    // Construct a scenario where the block is exactly the right size.
    HeapAllocator alloc(256);
    // Payload = 192.  Request 192 → exact fit, no split.
    void* p = alloc.allocate(192);
    ASSERT_NOT_NULL(p);

    auto stats = alloc.getStats();
    ASSERT_EQ(stats.total_blocks, size_t(1));
    ASSERT_EQ(stats.free_blocks, size_t(0));
    alloc.deallocate(p);
}

// ---- Minimum split boundary ----

TEST(Split_MinimumBoundary) {
    // Payload = 256 (from a 320-byte heap).
    // Request 64: remaining = 256 − 64 = 192 ≥ 128. → Split.
    HeapAllocator alloc(320);

    void* p = alloc.allocate(64);
    ASSERT_NOT_NULL(p);

    auto stats = alloc.getStats();
    ASSERT_EQ(stats.total_blocks, size_t(2));
    ASSERT_EQ(stats.free_blocks, size_t(1));
    // Free block payload: 192 − 64 (header) = 128
    ASSERT_EQ(stats.free_bytes, size_t(128));
    alloc.deallocate(p);
}
