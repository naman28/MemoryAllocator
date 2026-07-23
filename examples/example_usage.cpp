/// @file example_usage.cpp
/// @brief Demonstrates usage of BumpAllocator, HeapAllocator, and StlAllocator.

#include "allocator.h"
#include "bump_allocator.h"
#include "stl_adapter.h"

#include <iostream>
#include <vector>

using namespace mem;

int main() {
    std::cout << "╔══════════════════════════════════════╗\n"
              << "║   Custom Memory Allocator Demo       ║\n"
              << "╚══════════════════════════════════════╝\n\n";

    // =====================================================================
    // 1. Bump Allocator
    // =====================================================================
    {
        std::cout << "─── Bump Allocator ───\n\n";
        BumpAllocator bump(1024);

        int*    a = static_cast<int*>(bump.allocate(sizeof(int)));
        double* b = static_cast<double*>(bump.allocate(sizeof(double)));
        *a = 42;
        *b = 3.14159;

        std::cout << "  int   @ " << a << " = " << *a << "\n";
        std::cout << "  double@ " << b << " = " << *b << "\n";
        std::cout << "  Used  : " << bump.used() << " / " << bump.capacity() << " B\n";

        bump.reset();
        std::cout << "  After reset: " << bump.used() << " / " << bump.capacity() << " B\n\n";
    }

    // =====================================================================
    // 2. Heap Allocator — manual alloc / free / realloc
    // =====================================================================
    {
        std::cout << "─── Heap Allocator ───\n\n";
        HeapAllocator alloc(4096);

        void* p1 = alloc.allocate(128);
        void* p2 = alloc.allocate(256);
        void* p3 = alloc.allocate(64);

        std::cout << "  Allocated 3 blocks (128, 256, 64 B)\n";
        alloc.dumpHeap(std::cout);

        // Free middle block → creates a hole.
        alloc.deallocate(p2);
        std::cout << "  After freeing middle block (256 B):\n";
        alloc.dumpHeap(std::cout);

        // Allocate smaller — reuses the freed space.
        void* p4 = alloc.allocate(64);
        std::cout << "  After allocating 64 B (reuse hole):\n";
        alloc.dumpHeap(std::cout);

        // Reallocate — may expand in-place or move.
        static_cast<int*>(p1)[0] = 12345;
        void* p5 = alloc.reallocate(p1, 256);
        std::cout << "  After reallocating p1 from 128 B → 256 B:\n";
        std::cout << "  Data preserved: " << static_cast<int*>(p5)[0] << "\n";
        alloc.dumpHeap(std::cout);

        // Free everything.
        alloc.deallocate(p3);
        alloc.deallocate(p4);
        alloc.deallocate(p5);

        std::cout << "  After freeing all (should coalesce to 1 block):\n";
        alloc.dumpHeap(std::cout);
    }

    // =====================================================================
    // 3. STL Allocator Adapter
    // =====================================================================
    {
        std::cout << "─── STL Allocator Adapter ───\n\n";
        HeapAllocator heap(1048576);  // 1 MB
        StlAllocator<int> stlAlloc(heap);

        std::vector<int, StlAllocator<int>> vec(stlAlloc);
        for (int i = 0; i < 100; ++i) {
            vec.push_back(i * i);
        }

        std::cout << "  std::vector size: " << vec.size() << "\n";
        std::cout << "  First 5 squares : ";
        for (int i = 0; i < 5; ++i) std::cout << vec[i] << " ";
        std::cout << "\n\n";

        auto stats = heap.getStats();
        std::cout << "  Allocator stats after vector ops:\n";
        printStats(stats, std::cout);
    }

    std::cout << "╔══════════════════════════════════════╗\n"
              << "║   Demo Complete                      ║\n"
              << "╚══════════════════════════════════════╝\n";

    return 0;
}
