/// @file test_stats.cpp
/// @brief Milestone 7: Statistics and heap-dump tests.

#include "test_framework.h"
#include "allocator.h"

#include <sstream>

using namespace mem;

// ---- Initial state ----

TEST(Stats_InitialState) {
    HeapAllocator alloc(4096);
    auto stats = alloc.getStats();

    ASSERT_EQ(stats.total_capacity, size_t(4096));
    ASSERT_EQ(stats.allocated_bytes, size_t(0));
    ASSERT_EQ(stats.total_blocks, size_t(1));
    ASSERT_EQ(stats.free_blocks, size_t(1));
    ASSERT_EQ(stats.allocation_count, size_t(0));
    ASSERT_EQ(stats.deallocation_count, size_t(0));

    // free_bytes = capacity − header = 4032
    ASSERT_EQ(stats.free_bytes, size_t(4032));
    ASSERT_EQ(stats.overhead_bytes, sizeof(BlockHeader));
}

// ---- After allocations ----

TEST(Stats_AfterAllocs) {
    HeapAllocator alloc(4096);

    (void)alloc.allocate(64);
    (void)alloc.allocate(128);
    (void)alloc.allocate(64);

    auto stats = alloc.getStats();
    ASSERT_EQ(stats.allocation_count, size_t(3));
    ASSERT_EQ(stats.deallocation_count, size_t(0));
    ASSERT_EQ(stats.allocated_bytes, size_t(64 + 128 + 64));
}

// ---- After free ----

TEST(Stats_AfterFree) {
    HeapAllocator alloc(4096);

    void* p1 = alloc.allocate(64);
    void* p2 = alloc.allocate(128);

    alloc.deallocate(p1);

    auto stats = alloc.getStats();
    ASSERT_EQ(stats.allocation_count, size_t(2));
    ASSERT_EQ(stats.deallocation_count, size_t(1));
    // p1 freed, p2 still allocated.
    ASSERT_EQ(stats.allocated_bytes, size_t(128));

    alloc.deallocate(p2);
}

// ---- Fragmentation ratio ----

TEST(Stats_Fragmentation) {
    HeapAllocator alloc(4096);

    void* p1 = alloc.allocate(64);
    void* p2 = alloc.allocate(64);
    void* p3 = alloc.allocate(64);

    // Create a "hole": free p1 and p3.
    alloc.deallocate(p1);
    alloc.deallocate(p3);

    auto stats = alloc.getStats();
    // Two separate free blocks → fragmentation > 0.
    ASSERT_TRUE(stats.fragmentation_ratio > 0.0);
    ASSERT_TRUE(stats.fragmentation_ratio < 1.0);

    alloc.deallocate(p2);

    // After freeing p2, everything coalesces → fragmentation = 0.
    stats = alloc.getStats();
    ASSERT_TRUE(stats.fragmentation_ratio < 0.01);
}

// ---- allocatedMemory / freeMemory convenience ----

TEST(Stats_ConvenienceAccessors) {
    HeapAllocator alloc(4096);

    ASSERT_EQ(alloc.freeMemory(), size_t(4032));
    ASSERT_EQ(alloc.allocatedMemory(), size_t(0));

    void* p = alloc.allocate(64);
    ASSERT_GE(alloc.allocatedMemory(), size_t(64));
    ASSERT_TRUE(alloc.freeMemory() < 4032);

    alloc.deallocate(p);
}

// ---- dumpHeap produces output ----

TEST(Stats_DumpHeap) {
    HeapAllocator alloc(1024);
    (void)alloc.allocate(64);
    (void)alloc.allocate(128);

    std::ostringstream oss;
    alloc.dumpHeap(oss);

    std::string dump = oss.str();
    // Should contain key markers.
    ASSERT_TRUE(dump.find("Heap Dump") != std::string::npos);
    ASSERT_TRUE(dump.find("USED") != std::string::npos);
    ASSERT_TRUE(dump.find("FREE") != std::string::npos);
    ASSERT_TRUE(dump.find("Heap Statistics") != std::string::npos);
}
