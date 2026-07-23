#pragma once
/// @file allocator.h
/// @brief HeapAllocator — the main custom memory allocator.
///
/// Manages a single contiguous heap region using an explicit free list
/// with block splitting, coalescing, alignment support, reallocation,
/// error detection, and statistics.
///
/// Design decisions:
///   - 64-byte aligned blocks (cache-line sized headers)
///   - Segregated free lists for O(1) best-case small allocations
///   - Immediate coalescing on free to reduce fragmentation
///   - Canary-based corruption detection (BLOCK_MAGIC)
///   - Detects double-free, invalid-free, and null-free

#include "heap.h"
#include "freelist.h"
#include "stats.h"

#include <cstddef>
#include <iosfwd>

namespace mem {

class HeapAllocator {
public:
    /// Construct an allocator managing `capacity` bytes of heap.
    ///
    /// @param capacity  Total heap size in bytes (default 1 MB).
    ///                  Rounded up to BLOCK_ALIGNMENT internally.
    explicit HeapAllocator(size_t capacity = 1048576);

    ~HeapAllocator() = default;

    // Non-copyable, non-movable (owns heap + internal state).
    HeapAllocator(const HeapAllocator&)            = delete;
    HeapAllocator& operator=(const HeapAllocator&) = delete;
    HeapAllocator(HeapAllocator&&)                 = delete;
    HeapAllocator& operator=(HeapAllocator&&)      = delete;

    // ----- Core API -----

    /// Allocate `bytes` of memory (aligned to BLOCK_ALIGNMENT = 64).
    /// @return  Pointer to the payload, or nullptr on failure.
    [[nodiscard]] void* allocate(size_t bytes);

    /// Allocate `bytes` of memory with a specific alignment.
    /// @param alignment  Must be a power of 2, ≤ BLOCK_ALIGNMENT.
    /// @return  Pointer to the aligned payload, or nullptr on failure.
    [[nodiscard]] void* allocate(size_t bytes, size_t alignment);

    /// Free a previously-allocated pointer.
    /// @throws std::invalid_argument  If `ptr` is not owned by this allocator.
    /// @throws std::runtime_error     On double-free or corrupted metadata.
    void deallocate(void* ptr);

    /// Resize an allocation (shrink, grow in-place, or alloc+copy+free).
    /// @return  Pointer to the (possibly moved) payload, or nullptr on failure.
    [[nodiscard]] void* reallocate(void* ptr, size_t newSize);

    // ----- Statistics -----

    /// Total bytes currently allocated (sum of allocated-block payloads).
    [[nodiscard]] size_t allocatedMemory() const noexcept;

    /// Total bytes currently free (sum of free-block payloads).
    [[nodiscard]] size_t freeMemory() const noexcept;

    /// Full statistics snapshot (walks the entire block list).
    [[nodiscard]] HeapStats getStats() const;

    /// Print a visual dump of the entire heap to `os`.
    void dumpHeap(std::ostream& os) const;

private:
    /// Split `block` so that its payload becomes exactly `requestedSize`.
    /// The remainder becomes a new free block (if large enough).
    void splitBlock(BlockHeader* block, size_t requestedSize);

    /// Merge `block` with physically adjacent free neighbours.
    /// @return  The (possibly enlarged) block after merging.
    BlockHeader* coalesce(BlockHeader* block);

    /// Returns true if `ptr` is within [heap.base, heap.base + capacity).
    [[nodiscard]] bool isOwnedPointer(const void* ptr) const noexcept;

    Heap         heap_;
    FreeList     freeList_;
    BlockHeader* firstBlock_       = nullptr;

    // Cumulative counters
    size_t       allocationCount_   = 0;
    size_t       deallocationCount_ = 0;
};

} // namespace mem
