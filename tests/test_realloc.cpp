/// @file test_realloc.cpp
/// @brief Milestone 6: Reallocation tests.

#include "test_framework.h"
#include "allocator.h"

#include <cstring>

using namespace mem;

// ---- Shrink in-place ----

TEST(Realloc_ShrinkInPlace) {
    HeapAllocator alloc(4096);
    int* p = static_cast<int*>(alloc.allocate(256));
    ASSERT_NOT_NULL(p);
    p[0] = 42;

    int* p2 = static_cast<int*>(alloc.reallocate(p, 64));
    // Shrinking should return the same pointer (in-place).
    ASSERT_EQ(static_cast<void*>(p2), static_cast<void*>(p));
    ASSERT_EQ(p2[0], 42);  // Data preserved.
    alloc.deallocate(p2);
}

// ---- Grow via alloc + copy + free ----

TEST(Realloc_Grow) {
    HeapAllocator alloc(4096);
    int* p = static_cast<int*>(alloc.allocate(64));
    ASSERT_NOT_NULL(p);
    p[0] = 100;

    int* p2 = static_cast<int*>(alloc.reallocate(p, 256));
    ASSERT_NOT_NULL(p2);
    ASSERT_EQ(p2[0], 100);  // Data preserved after move.
    alloc.deallocate(p2);
}

// ---- Grow in-place (merge with free next) ----

TEST(Realloc_GrowInPlace) {
    HeapAllocator alloc(4096);
    void* p1 = alloc.allocate(64);
    void* p2 = alloc.allocate(64);
    ASSERT_NOT_NULL(p1);
    ASSERT_NOT_NULL(p2);

    // Free p2 → the block after p1 is now free.
    alloc.deallocate(p2);

    // Grow p1 — should expand in-place by absorbing freed p2.
    static_cast<int*>(p1)[0] = 999;
    void* p3 = alloc.reallocate(p1, 128);
    ASSERT_EQ(p3, p1);  // Same address → in-place expansion.
    ASSERT_EQ(static_cast<int*>(p3)[0], 999);
    alloc.deallocate(p3);
}

// ---- reallocate(nullptr, n) == allocate(n) ----

TEST(Realloc_NullPtrAllocates) {
    HeapAllocator alloc(4096);
    void* p = alloc.reallocate(nullptr, 128);
    ASSERT_NOT_NULL(p);
    alloc.deallocate(p);
}

// ---- reallocate(ptr, 0) == deallocate ----

TEST(Realloc_ZeroSizeFrees) {
    HeapAllocator alloc(4096);
    void* p = alloc.allocate(128);
    ASSERT_NOT_NULL(p);

    void* r = alloc.reallocate(p, 0);
    ASSERT_NULL(r);

    // Block should be freed — allocating again should reuse it.
    void* p2 = alloc.allocate(128);
    ASSERT_NOT_NULL(p2);
    alloc.deallocate(p2);
}

// ---- Data preservation across realloc ----

TEST(Realloc_DataPreservation) {
    HeapAllocator alloc(8192);
    constexpr size_t N = 16;

    auto* p = static_cast<uint8_t*>(alloc.allocate(N));
    ASSERT_NOT_NULL(p);
    for (size_t i = 0; i < N; ++i) p[i] = static_cast<uint8_t>(i);

    // Grow substantially — likely alloc+copy+free.
    auto* p2 = static_cast<uint8_t*>(alloc.reallocate(p, N * 4));
    ASSERT_NOT_NULL(p2);

    for (size_t i = 0; i < N; ++i) {
        ASSERT_EQ(p2[i], static_cast<uint8_t>(i));
    }
    alloc.deallocate(p2);
}

// ---- Realloc shrink creates free remainder ----

TEST(Realloc_ShrinkCreatesRemainder) {
    HeapAllocator alloc(4096);

    // Allocate a block large enough to shrink and split.
    void* p = alloc.allocate(512);
    ASSERT_NOT_NULL(p);

    auto before = alloc.getStats();
    void* p2 = alloc.reallocate(p, 64);
    ASSERT_NOT_NULL(p2);

    auto after = alloc.getStats();
    // The shrink should have created a new free block.
    ASSERT_TRUE(after.free_blocks >= before.free_blocks);
    alloc.deallocate(p2);
}
