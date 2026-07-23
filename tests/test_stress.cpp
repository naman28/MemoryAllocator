/// @file test_stress.cpp
/// @brief Randomized stress test — 1M operations with integrity checks.

#include "test_framework.h"
#include "allocator.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <random>
#include <vector>

using namespace mem;

// ---- Pattern fill / verify helpers ----

/// Write a repeating byte pattern into a buffer.
static void fillPattern(void* ptr, size_t size, uint8_t seed) {
    auto* p = static_cast<uint8_t*>(ptr);
    for (size_t i = 0; i < size; ++i) {
        p[i] = static_cast<uint8_t>((seed + i) & 0xFF);
    }
}

/// Verify the repeating byte pattern.
static bool verifyPattern(const void* ptr, size_t size, uint8_t seed) {
    auto* p = static_cast<const uint8_t*>(ptr);
    for (size_t i = 0; i < size; ++i) {
        if (p[i] != static_cast<uint8_t>((seed + i) & 0xFF)) {
            return false;
        }
    }
    return true;
}

// ---- Large random workload ----

TEST(Stress_RandomWorkload) {
    constexpr size_t HEAP_SIZE = 16 * 1024 * 1024;  // 16 MB
    constexpr size_t NUM_OPS  = 1'000'000;
    constexpr size_t MAX_LIVE = 10'000;

    HeapAllocator alloc(HEAP_SIZE);
    std::mt19937 rng(42);  // Fixed seed for reproducibility.

    struct Allocation {
        void*   ptr;
        size_t  size;
        uint8_t seed;
    };
    std::vector<Allocation> live;
    live.reserve(MAX_LIVE);

    std::uniform_int_distribution<size_t> sizeDist(1, 4096);
    std::uniform_int_distribution<int>    opDist(0, 99);

    for (size_t op = 0; op < NUM_OPS; ++op) {
        int choice = opDist(rng);

        if (choice < 60 || live.empty()) {
            // 60 %: allocate
            size_t sz   = sizeDist(rng);
            void*  ptr  = alloc.allocate(sz);
            if (ptr) {
                uint8_t seed = static_cast<uint8_t>(op & 0xFF);
                fillPattern(ptr, sz, seed);
                live.push_back({ptr, sz, seed});
            }
        } else if (choice < 90) {
            // 30 %: free a random live allocation
            std::uniform_int_distribution<size_t> idxDist(0, live.size() - 1);
            size_t idx = idxDist(rng);

            // Verify data integrity before freeing.
            ASSERT_TRUE(verifyPattern(live[idx].ptr, live[idx].size, live[idx].seed));

            alloc.deallocate(live[idx].ptr);
            live[idx] = live.back();
            live.pop_back();
        } else {
            // 10 %: reallocate a random live allocation
            std::uniform_int_distribution<size_t> idxDist(0, live.size() - 1);
            size_t idx     = idxDist(rng);
            size_t newSize = sizeDist(rng);

            // Verify data before realloc.
            ASSERT_TRUE(verifyPattern(live[idx].ptr, live[idx].size, live[idx].seed));

            void* newPtr = alloc.reallocate(live[idx].ptr, newSize);
            if (newPtr) {
                // Verify data preservation (up to min of old/new aligned sizes).
                size_t checkSize = std::min(live[idx].size, newSize);
                ASSERT_TRUE(verifyPattern(newPtr, checkSize, live[idx].seed));

                // Re-fill with a new pattern.
                uint8_t newSeed = static_cast<uint8_t>(op & 0xFF);
                fillPattern(newPtr, newSize, newSeed);
                live[idx] = {newPtr, newSize, newSeed};
            }
            // If realloc returned nullptr, the old allocation is still valid.
        }

        // Periodically trim live set to stay under MAX_LIVE.
        while (live.size() > MAX_LIVE) {
            size_t idx = live.size() - 1;
            alloc.deallocate(live[idx].ptr);
            live.pop_back();
        }
    }

    // Free all remaining allocations.
    for (auto& a : live) {
        ASSERT_TRUE(verifyPattern(a.ptr, a.size, a.seed));
        alloc.deallocate(a.ptr);
    }
    live.clear();

    // Final sanity: everything should coalesce back to one large free block.
    auto stats = alloc.getStats();
    ASSERT_EQ(stats.total_blocks, size_t(1));
    ASSERT_EQ(stats.free_blocks, size_t(1));
    ASSERT_EQ(stats.allocated_bytes, size_t(0));
}

// ---- Fragmentation resistance ----

TEST(Stress_FragmentationPattern) {
    HeapAllocator alloc(1048576);  // 1 MB

    // Allocate a mix of sizes.
    std::vector<void*> ptrs;
    size_t sizes[] = {64, 128, 256, 512, 64, 128, 256, 512};
    for (size_t s : sizes) {
        void* p = alloc.allocate(s);
        ASSERT_NOT_NULL(p);
        ptrs.push_back(p);
    }

    // Free every other allocation (create fragmentation).
    for (size_t i = 1; i < ptrs.size(); i += 2) {
        alloc.deallocate(ptrs[i]);
        ptrs[i] = nullptr;
    }

    auto stats = alloc.getStats();
    ASSERT_TRUE(stats.free_blocks >= 1);

    // Free the rest — should coalesce.
    for (auto* p : ptrs) {
        if (p) alloc.deallocate(p);
    }

    stats = alloc.getStats();
    ASSERT_EQ(stats.total_blocks, size_t(1));
}

// ---- Alloc-free ping-pong (checks reuse) ----

TEST(Stress_PingPong) {
    HeapAllocator alloc(4096);
    for (int i = 0; i < 10000; ++i) {
        void* p = alloc.allocate(64);
        ASSERT_NOT_NULL(p);
        alloc.deallocate(p);
    }

    auto stats = alloc.getStats();
    ASSERT_EQ(stats.allocation_count, size_t(10000));
    ASSERT_EQ(stats.deallocation_count, size_t(10000));
    ASSERT_EQ(stats.total_blocks, size_t(1));
}
