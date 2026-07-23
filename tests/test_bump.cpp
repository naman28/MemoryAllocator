/// @file test_bump.cpp
/// @brief Milestone 1: BumpAllocator tests.

#include "test_framework.h"
#include "bump_allocator.h"

#include <cstdint>

using namespace mem;

// ---- Basic allocation ----

TEST(Bump_BasicAllocation) {
    BumpAllocator alloc(1024);

    void* p1 = alloc.allocate(64);
    ASSERT_NOT_NULL(p1);

    void* p2 = alloc.allocate(128);
    ASSERT_NOT_NULL(p2);

    // Pointers must be distinct and sequential (bump goes forward).
    ASSERT_NE(p1, p2);
    ASSERT_TRUE(reinterpret_cast<uintptr_t>(p2) > reinterpret_cast<uintptr_t>(p1));
}

// ---- Alignment ----

TEST(Bump_Alignment_16) {
    BumpAllocator alloc(1024);
    void* p = alloc.allocate(1, 16);
    ASSERT_NOT_NULL(p);
    ASSERT_EQ(reinterpret_cast<uintptr_t>(p) % 16, uintptr_t(0));
}

TEST(Bump_Alignment_32) {
    BumpAllocator alloc(1024);
    void* p = alloc.allocate(1, 32);
    ASSERT_NOT_NULL(p);
    ASSERT_EQ(reinterpret_cast<uintptr_t>(p) % 32, uintptr_t(0));
}

TEST(Bump_Alignment_64) {
    BumpAllocator alloc(1024);
    void* p = alloc.allocate(1, 64);
    ASSERT_NOT_NULL(p);
    ASSERT_EQ(reinterpret_cast<uintptr_t>(p) % 64, uintptr_t(0));
}

// ---- Used / Remaining ----

TEST(Bump_UsedRemaining) {
    BumpAllocator alloc(1024);
    ASSERT_EQ(alloc.used(), size_t(0));
    ASSERT_EQ(alloc.remaining(), alloc.capacity());

    (void)alloc.allocate(100);
    ASSERT_GE(alloc.used(), size_t(100));
    ASSERT_LE(alloc.remaining(), alloc.capacity() - 100);
}

// ---- Out of memory ----

TEST(Bump_OOM) {
    BumpAllocator alloc(128);
    void* p = alloc.allocate(256);
    ASSERT_NULL(p);
}

TEST(Bump_OOM_GradualExhaustion) {
    BumpAllocator alloc(256);
    // Allocate until we run out.
    int count = 0;
    while (alloc.allocate(64)) {
        ++count;
    }
    ASSERT_TRUE(count > 0);
    // No more space.
    ASSERT_NULL(alloc.allocate(1));
}

// ---- Reset ----

TEST(Bump_Reset) {
    BumpAllocator alloc(1024);
    (void)alloc.allocate(512);
    ASSERT_GE(alloc.used(), size_t(512));

    alloc.reset();
    ASSERT_EQ(alloc.used(), size_t(0));
    ASSERT_EQ(alloc.remaining(), alloc.capacity());

    // Can allocate again after reset.
    void* p = alloc.allocate(256);
    ASSERT_NOT_NULL(p);
}

// ---- Zero-size allocation ----

TEST(Bump_ZeroSize) {
    BumpAllocator alloc(1024);
    void* p = alloc.allocate(0);
    ASSERT_NULL(p);
}
