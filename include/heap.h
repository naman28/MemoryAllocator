#pragma once
/// @file heap.h
/// @brief RAII wrapper for the raw heap memory region.
///
/// The Heap reserves a large, aligned block of memory once at construction
/// and releases it on destruction.  All allocator logic lives elsewhere;
/// the Heap simply owns the underlying byte range.

#include "block.h"

#include <cstddef>
#include <cstdlib>
#include <new>
#include <stdexcept>

namespace mem {

class Heap {
public:
    /// Reserve `capacity` bytes of BLOCK_ALIGNMENT-aligned memory.
    ///
    /// @param capacity  Requested capacity in bytes (rounded up to BLOCK_ALIGNMENT).
    /// @throws std::invalid_argument  If `capacity` is too small for even one block.
    /// @throws std::bad_alloc         If the underlying allocation fails.
    explicit Heap(size_t capacity);

    /// Release the reserved memory.
    ~Heap();

    // Non-copyable, non-movable (unique owner of the raw region).
    Heap(const Heap&)            = delete;
    Heap& operator=(const Heap&) = delete;
    Heap(Heap&&)                 = delete;
    Heap& operator=(Heap&&)      = delete;

    /// Base address of the reserved region.
    [[nodiscard]] std::byte*       base()     noexcept { return base_; }
    [[nodiscard]] const std::byte* base()     const noexcept { return base_; }

    /// Actual capacity after alignment rounding.
    [[nodiscard]] size_t           capacity() const noexcept { return capacity_; }

    /// Returns true if `ptr` falls within [base, base + capacity).
    [[nodiscard]] bool contains(const void* ptr) const noexcept;

private:
    std::byte* base_     = nullptr;
    size_t     capacity_ = 0;
};

} // namespace mem
