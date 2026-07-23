/// @file test_coalescing.cpp
/// @brief Milestone 4: Block-coalescing tests.
///
/// Verifies that adjacent free blocks are merged into a single larger block,
/// reducing external fragmentation.

#include "test_framework.h"
#include "allocator.h"

using namespace mem;

// ---- Two adjacent blocks merge ----

TEST(Coalesce_TwoAdjacent) {
    HeapAllocator alloc(512);
    // State after 3 allocs (each 64 B payload):
    //   Block 0 [USED 64]  Block 1 [USED 64]  Block 2 [USED 64]  Block 3 [FREE 64]

    void* p1 = alloc.allocate(64);
    void* p2 = alloc.allocate(64);
    void* p3 = alloc.allocate(64);
    ASSERT_NOT_NULL(p1);
    ASSERT_NOT_NULL(p2);
    ASSERT_NOT_NULL(p3);

    // Free p1, then p2 — p2's predecessor (p1) is free → merge.
    alloc.deallocate(p1);
    alloc.deallocate(p2);

    auto stats = alloc.getStats();
    // Expected: [FREE 192] [USED 64] [FREE 64]
    //   Merged block = p1(64) + header(64) + p2(64) = 192
    ASSERT_EQ(stats.free_blocks, size_t(2));
    ASSERT_EQ(stats.largest_free_block, size_t(192));
    alloc.deallocate(p3);
}

// ---- Three adjacent blocks merge (triple coalesce) ----

TEST(Coalesce_ThreeBlocks) {
    HeapAllocator alloc(512);
    void* p1 = alloc.allocate(64);
    void* p2 = alloc.allocate(64);
    void* p3 = alloc.allocate(64);
    ASSERT_NOT_NULL(p1);
    ASSERT_NOT_NULL(p2);
    ASSERT_NOT_NULL(p3);

    // Free p1 and p3 first (non-adjacent, p2 still in the middle).
    alloc.deallocate(p1);
    alloc.deallocate(p3);  // p3 merges with Block 3 (the remainder)

    // Now free p2 — it sits between two free blocks → triple merge.
    alloc.deallocate(p2);

    auto stats = alloc.getStats();
    // All blocks should coalesce back into the original single free block.
    //   Original payload = 512 − 64 = 448.
    ASSERT_EQ(stats.free_blocks, size_t(1));
    ASSERT_EQ(stats.total_blocks, size_t(1));
    ASSERT_EQ(stats.largest_free_block, size_t(448));
}

// ---- Non-adjacent blocks stay separate ----

TEST(Coalesce_NonAdjacent) {
    HeapAllocator alloc(4096);
    void* p1 = alloc.allocate(64);
    void* p2 = alloc.allocate(64);
    void* p3 = alloc.allocate(64);
    ASSERT_NOT_NULL(p1);
    ASSERT_NOT_NULL(p2);
    ASSERT_NOT_NULL(p3);

    // Free p1 and p3 — p2 stays allocated between them → no merge.
    alloc.deallocate(p1);
    alloc.deallocate(p3);

    auto stats = alloc.getStats();
    // [FREE] [USED p2] [FREE (p3 merged with remainder)]
    ASSERT_EQ(stats.free_blocks, size_t(2));
    ASSERT_EQ(stats.total_blocks, size_t(3));

    alloc.deallocate(p2);
}

// ---- Free last block merges with successor ----

TEST(Coalesce_MergeWithNext) {
    HeapAllocator alloc(4096);
    void* p1 = alloc.allocate(64);
    void* p2 = alloc.allocate(64);
    ASSERT_NOT_NULL(p1);
    ASSERT_NOT_NULL(p2);

    // Free p2 — the block after p2 (remainder) is free → merge with next.
    alloc.deallocate(p2);

    auto stats = alloc.getStats();
    // [USED p1] [FREE (p2 + remainder)]
    ASSERT_EQ(stats.free_blocks, size_t(1));
    ASSERT_EQ(stats.total_blocks, size_t(2));

    alloc.deallocate(p1);
}

// ---- Full round-trip: alloc all → free all → single block ----

TEST(Coalesce_FullRoundTrip) {
    HeapAllocator alloc(1024);
    constexpr int N = 5;
    void* ptrs[N];

    for (int i = 0; i < N; ++i) {
        ptrs[i] = alloc.allocate(64);
        ASSERT_NOT_NULL(ptrs[i]);
    }

    // Free in reverse order — each free merges with the successor.
    for (int i = N - 1; i >= 0; --i) {
        alloc.deallocate(ptrs[i]);
    }

    auto stats = alloc.getStats();
    // Everything should coalesce back to one block.
    ASSERT_EQ(stats.total_blocks, size_t(1));
    ASSERT_EQ(stats.free_blocks, size_t(1));
    // Payload = 1024 − 64 = 960.
    ASSERT_EQ(stats.largest_free_block, size_t(960));
}
