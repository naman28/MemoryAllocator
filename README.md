# High-Performance Custom Memory Allocator

A production-quality custom memory allocator in Modern C++ (C++20) that manages raw heap memory with minimal fragmentation, low latency, and high throughput.

## Features

- **Manual heap management** — reserves memory once, manages all allocations internally
- **Dynamic allocation** — `allocate()`, `deallocate()`, `reallocate()` APIs
- **Memory reuse** — freed blocks are returned to a segregated free list
- **Block splitting** — large blocks are split to reduce waste
- **Block coalescing** — adjacent free blocks merge automatically to reduce fragmentation
- **Configurable alignment** — supports 8, 16, 32, and 64-byte alignment
- **Error detection** — double-free, invalid-free, null-free, and corruption detection via canary values
- **Statistics & visualization** — `getStats()` and `dumpHeap()` for debugging
- **STL integration** — `StlAllocator<T>` adapter for `std::vector`, `std::list`, `std::map`, etc.
- **Comprehensive testing** — 59 tests including randomized stress tests with 1M operations

## Quick Start

```bash
# Build and run all tests (debug mode with ASan + UBSan)
make test MODE=debug

# Build and run all tests (release mode)
make test

# Run benchmarks
make bench

# Run example
make example

# Build everything
make all

# Clean build artifacts
make clean
```

## Usage

```cpp
#include "allocator.h"

int main() {
    mem::HeapAllocator alloc(1048576);  // 1 MB heap

    // Allocate
    void* p1 = alloc.allocate(128);
    void* p2 = alloc.allocate(256);

    // Reallocate (may expand in-place)
    void* p3 = alloc.reallocate(p1, 512);

    // Free
    alloc.deallocate(p2);
    alloc.deallocate(p3);

    // Statistics
    alloc.dumpHeap(std::cout);
}
```

### STL Integration

```cpp
#include "stl_adapter.h"

mem::HeapAllocator heap(1048576);
mem::StlAllocator<int> alloc(heap);

std::vector<int, mem::StlAllocator<int>> vec(alloc);
vec.push_back(42);
```

## Architecture

```
include/
├── allocator.h        Main HeapAllocator class
├── block.h            BlockHeader metadata (64B cache-line aligned)
├── freelist.h         Segregated free-list manager (4 size classes)
├── heap.h             Raw heap region (RAII)
├── bump_allocator.h   Simple bump allocator (learning tool)
├── stats.h            HeapStats struct
└── stl_adapter.h      STL allocator adapter

src/                   Implementation files
tests/                 59 unit + stress tests
benchmark/             Performance comparison vs malloc
examples/              Usage demonstrations
docs/                  Design documentation
```

## Design

- **Block size**: 64-byte headers (one cache line), payloads rounded to 64B
- **Free list**: 4 segregated size classes (tiny ≤64, small ≤256, medium ≤1024, large)
- **Allocation**: First-fit search cascading through size classes
- **Deallocation**: Immediate coalescing with adjacent free blocks
- **Alignment**: All payloads are 64-byte aligned (supports 8/16/32/64)
- **Safety**: `0xDEADBEEF` canary in every header, validated on every free

## Requirements

- C++20 compiler (tested with Apple Clang 17)
- `make`

## License

MIT
