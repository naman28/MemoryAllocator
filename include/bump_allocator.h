#pragma once
/// @file bump_allocator.h
/// @brief Bump (arena-style) allocator — the simplest possible allocator.
///
/// Allocations advance a cursor forward through a pre-reserved region.
/// Individual deallocations are not supported; only a bulk `reset()` is
/// provided.  This makes it ideal for phase-based workloads where all
/// allocations are freed at once (e.g. per-frame game allocations).
///
/// This is Milestone 1 — it teaches pointer arithmetic, alignment, and
/// the concept of managing a raw memory region without `new` / `malloc`.

#include <cstddef>
#include <cstdlib>
#include <new>

namespace mem {

class BumpAllocator {
public:
    /// Reserve `capacity` bytes of memory.
    /// @throws std::bad_alloc if the reservation fails.
    explicit BumpAllocator(size_t capacity);

    /// Release the reserved memory.
    ~BumpAllocator();

    // Non-copyable (owns the raw region).
    BumpAllocator(const BumpAllocator&)            = delete;
    BumpAllocator& operator=(const BumpAllocator&) = delete;

    /// Allocate `bytes` with the given alignment.
    /// @return  Pointer to the aligned allocation, or nullptr if OOM.
    [[nodiscard]] void* allocate(size_t bytes,
                                 size_t alignment = alignof(std::max_align_t));

    /// Reset the cursor to the start (bulk-free all allocations).
    void reset() noexcept;

    /// Number of bytes currently in use.
    [[nodiscard]] size_t used()      const noexcept;

    /// Number of bytes remaining.
    [[nodiscard]] size_t remaining() const noexcept;

    /// Total capacity of the reserved region.
    [[nodiscard]] size_t capacity()  const noexcept;

private:
    std::byte* base_    = nullptr;
    std::byte* current_ = nullptr;
    std::byte* end_     = nullptr;
    size_t     capacity_ = 0;
};

} // namespace mem
