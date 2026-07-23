/// @file test_freelist.cpp
/// @brief Milestone 2: Free-list allocator core tests.

#include "test_framework.h"
#include "allocator.h"

#include <cstdint>
#include <stdexcept>

using namespace mem;

// ---- Basic allocate / free ----

TEST(FreeList_AllocAndFree) {
    HeapAllocator alloc(4096);
    void* p = alloc.allocate(64);
    ASSERT_NOT_NULL(p);
    alloc.deallocate(p);  // Should not throw.
}

// ---- Memory reuse ----

TEST(FreeList_Reuse) {
    HeapAllocator alloc(4096);
    void* p1 = alloc.allocate(64);
    ASSERT_NOT_NULL(p1);
    alloc.deallocate(p1);

    void* p2 = alloc.allocate(64);
    ASSERT_NOT_NULL(p2);
    // The freed block should be reused (same address).
    ASSERT_EQ(p1, p2);
    alloc.deallocate(p2);
}

// ---- Multiple allocations ----

TEST(FreeList_MultipleAllocs) {
    HeapAllocator alloc(4096);
    constexpr int N = 10;
    void* ptrs[N];

    for (int i = 0; i < N; ++i) {
        ptrs[i] = alloc.allocate(64);
        ASSERT_NOT_NULL(ptrs[i]);
    }

    // All pointers must be unique.
    for (int i = 0; i < N; ++i) {
        for (int j = i + 1; j < N; ++j) {
            ASSERT_NE(ptrs[i], ptrs[j]);
        }
    }

    for (int i = 0; i < N; ++i) {
        alloc.deallocate(ptrs[i]);
    }
}

// ---- Data integrity ----

TEST(FreeList_DataIntegrity) {
    HeapAllocator alloc(4096);

    int* p = static_cast<int*>(alloc.allocate(sizeof(int) * 4));
    ASSERT_NOT_NULL(p);

    p[0] = 100; p[1] = 200; p[2] = 300; p[3] = 400;
    ASSERT_EQ(p[0], 100);
    ASSERT_EQ(p[1], 200);
    ASSERT_EQ(p[2], 300);
    ASSERT_EQ(p[3], 400);

    alloc.deallocate(p);
}

// ---- Double free detection ----

TEST(FreeList_DoubleFreeDetection) {
    HeapAllocator alloc(4096);
    void* p = alloc.allocate(64);
    alloc.deallocate(p);
    ASSERT_THROWS(alloc.deallocate(p), std::runtime_error);
}

// ---- Invalid pointer detection ----

TEST(FreeList_InvalidPointer) {
    HeapAllocator alloc(4096);
    int x = 42;
    ASSERT_THROWS(alloc.deallocate(&x), std::invalid_argument);
}

// ---- Null free is safe ----

TEST(FreeList_NullFreeIsSafe) {
    HeapAllocator alloc(4096);
    alloc.deallocate(nullptr);  // Must not throw.
}

// ---- Zero-size allocation ----

TEST(FreeList_ZeroSizeAlloc) {
    HeapAllocator alloc(4096);
    void* p = alloc.allocate(0);
    ASSERT_NULL(p);
}

// ---- Out of memory ----

TEST(FreeList_OOM) {
    HeapAllocator alloc(256);
    // Request more than the entire heap payload.
    void* p = alloc.allocate(1024);
    ASSERT_NULL(p);
}
