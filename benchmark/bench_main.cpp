/// @file bench_main.cpp
/// @brief Benchmark: Custom allocator vs malloc/free.
///
/// Measures allocation latency, deallocation latency, and throughput
/// for small, medium, large, and mixed workloads.

#include "allocator.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

using namespace mem;
using Clock = std::chrono::high_resolution_clock;

// ---------------------------------------------------------------------------
// Result
// ---------------------------------------------------------------------------

struct BenchResult {
    std::string name;
    double      avgAllocNs;
    double      avgFreeNs;
    double      throughputMops;   // million ops / second (alloc + free)
};

static void printResult(const BenchResult& r) {
    std::cout << std::left << std::setw(28) << r.name << std::right
              << "  alloc: " << std::setw(8) << std::fixed << std::setprecision(1)
              << r.avgAllocNs << " ns"
              << "  free: " << std::setw(8) << r.avgFreeNs << " ns"
              << "  throughput: " << std::setw(8) << std::setprecision(2)
              << r.throughputMops << " Mops/s"
              << "\n";
}

// ---------------------------------------------------------------------------
// Fixed-size benchmark
// ---------------------------------------------------------------------------

template <typename AllocFn, typename FreeFn>
static BenchResult benchFixed(const std::string& name, size_t count, size_t size,
                              AllocFn allocFn, FreeFn freeFn) {
    std::vector<void*> ptrs(count);

    auto t0 = Clock::now();
    for (size_t i = 0; i < count; ++i) {
        ptrs[i] = allocFn(size);
    }
    auto t1 = Clock::now();
    for (size_t i = 0; i < count; ++i) {
        freeFn(ptrs[i]);
    }
    auto t2 = Clock::now();

    double allocNs = std::chrono::duration<double, std::nano>(t1 - t0).count();
    double freeNs  = std::chrono::duration<double, std::nano>(t2 - t1).count();

    return {
        name,
        allocNs / static_cast<double>(count),
        freeNs  / static_cast<double>(count),
        (2.0 * count) / ((allocNs + freeNs) / 1e9) / 1e6
    };
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
    constexpr size_t N         = 100'000;
    constexpr size_t HEAP_SIZE = 64 * 1024 * 1024;  // 64 MB

    std::cout << "\n"
              << "╔══════════════════════════════════════════════╗\n"
              << "║      Memory Allocator Benchmark              ║\n"
              << "║      " << N << " ops per scenario"
              << std::string(22 - std::to_string(N).size(), ' ') << "║\n"
              << "╚══════════════════════════════════════════════╝\n\n";

    // ---- Small allocations (64 B) ----
    {
        HeapAllocator custom(HEAP_SIZE);
        auto r1 = benchFixed("Custom  (64 B)", N, 64,
                             [&](size_t s) { return custom.allocate(s); },
                             [&](void* p) { custom.deallocate(p); });
        auto r2 = benchFixed("malloc  (64 B)", N, 64,
                             [](size_t s) { return std::malloc(s); },
                             [](void* p) { std::free(p); });
        printResult(r1);
        printResult(r2);
        std::cout << "\n";
    }

    // ---- Medium allocations (256 B) ----
    {
        HeapAllocator custom(HEAP_SIZE);
        auto r1 = benchFixed("Custom  (256 B)", N, 256,
                             [&](size_t s) { return custom.allocate(s); },
                             [&](void* p) { custom.deallocate(p); });
        auto r2 = benchFixed("malloc  (256 B)", N, 256,
                             [](size_t s) { return std::malloc(s); },
                             [](void* p) { std::free(p); });
        printResult(r1);
        printResult(r2);
        std::cout << "\n";
    }

    // ---- Large allocations (4096 B) ----
    {
        HeapAllocator custom(HEAP_SIZE);
        auto r1 = benchFixed("Custom  (4096 B)", N, 4096,
                             [&](size_t s) { return custom.allocate(s); },
                             [&](void* p) { custom.deallocate(p); });
        auto r2 = benchFixed("malloc  (4096 B)", N, 4096,
                             [](size_t s) { return std::malloc(s); },
                             [](void* p) { std::free(p); });
        printResult(r1);
        printResult(r2);
        std::cout << "\n";
    }

    // ---- Mixed random workload ----
    {
        std::mt19937 rng(42);
        std::uniform_int_distribution<size_t> sizeDist(64, 4096);
        std::vector<size_t> sizes(N);
        for (auto& s : sizes) s = sizeDist(rng);

        // Custom
        {
            HeapAllocator custom(HEAP_SIZE);
            std::vector<void*> ptrs(N);

            auto t0 = Clock::now();
            for (size_t i = 0; i < N; ++i) ptrs[i] = custom.allocate(sizes[i]);
            auto t1 = Clock::now();
            for (size_t i = 0; i < N; ++i) custom.deallocate(ptrs[i]);
            auto t2 = Clock::now();

            double aNs = std::chrono::duration<double, std::nano>(t1 - t0).count();
            double fNs = std::chrono::duration<double, std::nano>(t2 - t1).count();

            printResult({"Custom  (mixed)", aNs / N, fNs / N,
                         (2.0 * N) / ((aNs + fNs) / 1e9) / 1e6});
        }

        // malloc
        {
            std::vector<void*> ptrs(N);

            auto t0 = Clock::now();
            for (size_t i = 0; i < N; ++i) ptrs[i] = std::malloc(sizes[i]);
            auto t1 = Clock::now();
            for (size_t i = 0; i < N; ++i) std::free(ptrs[i]);
            auto t2 = Clock::now();

            double aNs = std::chrono::duration<double, std::nano>(t1 - t0).count();
            double fNs = std::chrono::duration<double, std::nano>(t2 - t1).count();

            printResult({"malloc  (mixed)", aNs / N, fNs / N,
                         (2.0 * N) / ((aNs + fNs) / 1e9) / 1e6});
        }
        std::cout << "\n";
    }

    // ---- Alloc-free-reuse pattern ----
    {
        HeapAllocator custom(HEAP_SIZE);
        std::vector<void*> ptrs(N);

        auto t0 = Clock::now();
        for (size_t i = 0; i < N; ++i) ptrs[i] = custom.allocate(128);
        for (size_t i = 0; i < N; ++i) custom.deallocate(ptrs[i]);
        for (size_t i = 0; i < N; ++i) ptrs[i] = custom.allocate(128);
        for (size_t i = 0; i < N; ++i) custom.deallocate(ptrs[i]);
        auto t1 = Clock::now();

        double totalNs = std::chrono::duration<double, std::nano>(t1 - t0).count();
        std::cout << "Alloc-Free-Reuse (128 B, " << N << " ops):"
                  << "  avg " << std::fixed << std::setprecision(1)
                  << totalNs / (4.0 * N) << " ns/op\n";

        auto stats = custom.getStats();
        std::cout << "  Fragmentation: " << std::setprecision(2)
                  << stats.fragmentation_ratio * 100 << "%\n";
    }

    std::cout << "\n"
              << "══════════════════════════════════════════════\n"
              << "  Benchmark complete\n"
              << "══════════════════════════════════════════════\n\n";

    return 0;
}
