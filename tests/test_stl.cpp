/// @file test_stl.cpp
/// @brief Milestone 9: STL allocator adapter tests.

#include "test_framework.h"
#include "stl_adapter.h"

#include <algorithm>
#include <list>
#include <map>
#include <numeric>
#include <string>
#include <vector>

using namespace mem;

// ---- std::vector<int> ----

TEST(STL_VectorInt) {
    HeapAllocator heap(1048576);
    StlAllocator<int> alloc(heap);

    std::vector<int, StlAllocator<int>> vec(alloc);
    for (int i = 0; i < 100; ++i) {
        vec.push_back(i * i);
    }

    ASSERT_EQ(vec.size(), size_t(100));
    ASSERT_EQ(vec[0], 0);
    ASSERT_EQ(vec[10], 100);
    ASSERT_EQ(vec[99], 99 * 99);
}

// ---- std::vector<double> ----

TEST(STL_VectorDouble) {
    HeapAllocator heap(1048576);
    StlAllocator<double> alloc(heap);

    std::vector<double, StlAllocator<double>> vec(alloc);
    for (int i = 0; i < 50; ++i) {
        vec.push_back(i * 0.5);
    }

    ASSERT_EQ(vec.size(), size_t(50));
    ASSERT_TRUE(vec[0] < 0.001);
    ASSERT_TRUE(std::abs(vec[1] - 0.5) < 0.001);
}

// ---- std::list (node-based, tests rebind) ----

TEST(STL_List) {
    HeapAllocator heap(1048576);
    StlAllocator<int> alloc(heap);

    std::list<int, StlAllocator<int>> lst(alloc);
    for (int i = 0; i < 50; ++i) {
        lst.push_back(i);
    }

    ASSERT_EQ(lst.size(), size_t(50));
    ASSERT_EQ(lst.front(), 0);
    ASSERT_EQ(lst.back(), 49);
}

// ---- std::map (node-based, tests rebind with pair) ----

TEST(STL_Map) {
    HeapAllocator heap(1048576);
    StlAllocator<std::pair<const int, int>> alloc(heap);

    std::map<int, int, std::less<int>,
             StlAllocator<std::pair<const int, int>>> mp(alloc);

    for (int i = 0; i < 50; ++i) {
        mp[i] = i * 10;
    }

    ASSERT_EQ(mp.size(), size_t(50));
    ASSERT_EQ(mp[0], 0);
    ASSERT_EQ(mp[25], 250);
    ASSERT_EQ(mp[49], 490);
}

// ---- Vector resize / reserve ----

TEST(STL_VectorResize) {
    HeapAllocator heap(1048576);
    StlAllocator<int> alloc(heap);

    std::vector<int, StlAllocator<int>> vec(alloc);
    vec.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        vec.push_back(i);
    }

    ASSERT_EQ(vec.size(), size_t(1000));
    ASSERT_EQ(vec[999], 999);
}

// ---- Allocator equality ----

TEST(STL_AllocatorEquality) {
    HeapAllocator heap1(4096);
    HeapAllocator heap2(4096);
    StlAllocator<int> a1(heap1);
    StlAllocator<int> a2(heap1);
    StlAllocator<int> a3(heap2);

    ASSERT_TRUE(a1 == a2);   // Same underlying allocator.
    ASSERT_TRUE(a1 != a3);   // Different underlying allocator.
}

// ---- Verify allocations go through custom allocator ----

TEST(STL_AllocationsTracked) {
    HeapAllocator heap(1048576);
    StlAllocator<int> alloc(heap);

    {
        std::vector<int, StlAllocator<int>> vec(alloc);
        vec.push_back(1);
        vec.push_back(2);
        vec.push_back(3);

        auto stats = heap.getStats();
        ASSERT_TRUE(stats.allocation_count > 0);
        ASSERT_TRUE(stats.allocated_bytes > 0);
    }
    // Vector destructor should deallocate.
    auto stats = heap.getStats();
    ASSERT_TRUE(stats.deallocation_count > 0);
}
