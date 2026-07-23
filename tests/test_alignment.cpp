/// @file test_alignment.cpp
/// @brief Milestone 5: Alignment tests.
///
/// Verifies that every returned pointer satisfies the requested alignment.

#include "test_framework.h"
#include "allocator.h"

#include <cstdint>

using namespace mem;

// ---- Default alignment (64 = BLOCK_ALIGNMENT) ----

TEST(Align_Default64) {
    HeapAllocator alloc(4096);
    for (int i = 0; i < 10; ++i) {
        void* p = alloc.allocate(64);
        ASSERT_NOT_NULL(p);
        ASSERT_EQ(reinterpret_cast<uintptr_t>(p) % 64, uintptr_t(0));
    }
}

// ---- 8-byte alignment ----

TEST(Align_8Byte) {
    HeapAllocator alloc(4096);
    for (int i = 0; i < 10; ++i) {
        void* p = alloc.allocate(100, 8);
        ASSERT_NOT_NULL(p);
        ASSERT_EQ(reinterpret_cast<uintptr_t>(p) % 8, uintptr_t(0));
    }
}

// ---- 16-byte alignment ----

TEST(Align_16Byte) {
    HeapAllocator alloc(4096);
    for (int i = 0; i < 10; ++i) {
        void* p = alloc.allocate(100, 16);
        ASSERT_NOT_NULL(p);
        ASSERT_EQ(reinterpret_cast<uintptr_t>(p) % 16, uintptr_t(0));
    }
}

// ---- 32-byte alignment ----

TEST(Align_32Byte) {
    HeapAllocator alloc(4096);
    for (int i = 0; i < 10; ++i) {
        void* p = alloc.allocate(100, 32);
        ASSERT_NOT_NULL(p);
        ASSERT_EQ(reinterpret_cast<uintptr_t>(p) % 32, uintptr_t(0));
    }
}

// ---- 64-byte alignment (explicit) ----

TEST(Align_64Byte) {
    HeapAllocator alloc(4096);
    for (int i = 0; i < 10; ++i) {
        void* p = alloc.allocate(100, 64);
        ASSERT_NOT_NULL(p);
        ASSERT_EQ(reinterpret_cast<uintptr_t>(p) % 64, uintptr_t(0));
    }
}

// ---- Invalid alignment returns nullptr ----

TEST(Align_InvalidAlignment) {
    HeapAllocator alloc(4096);
    // Not a power of 2.
    ASSERT_NULL(alloc.allocate(64, 3));
    ASSERT_NULL(alloc.allocate(64, 7));
    // Zero alignment.
    ASSERT_NULL(alloc.allocate(64, 0));
    // Larger than BLOCK_ALIGNMENT.
    ASSERT_NULL(alloc.allocate(64, 128));
}

// ---- Mixed sizes with alignment ----

TEST(Align_MixedSizes) {
    HeapAllocator alloc(8192);
    size_t sizes[]      = {1, 7, 13, 33, 100, 255, 512, 1024};
    size_t alignments[] = {8, 16, 32, 64};

    for (size_t s : sizes) {
        for (size_t a : alignments) {
            void* p = alloc.allocate(s, a);
            ASSERT_NOT_NULL(p);
            ASSERT_EQ(reinterpret_cast<uintptr_t>(p) % a, uintptr_t(0));
            alloc.deallocate(p);
        }
    }
}
