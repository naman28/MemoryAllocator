/// @file bump_allocator.cpp
/// @brief BumpAllocator implementation.

#include "bump_allocator.h"

#include <cstdint>
#include <new>

namespace mem {

BumpAllocator::BumpAllocator(size_t capacity)
    : capacity_(capacity)
{
    // Use aligned_alloc so the region is max_align_t-aligned.
    size_t alignedCap = (capacity + alignof(std::max_align_t) - 1)
                      & ~(alignof(std::max_align_t) - 1);
    if (alignedCap == 0) alignedCap = alignof(std::max_align_t);
    capacity_ = alignedCap;

    base_ = static_cast<std::byte*>(
        std::aligned_alloc(alignof(std::max_align_t), capacity_));
    if (!base_) {
        throw std::bad_alloc();
    }

    current_ = base_;
    end_     = base_ + capacity_;
}

BumpAllocator::~BumpAllocator() {
    std::free(base_);
}

void* BumpAllocator::allocate(size_t bytes, size_t alignment) {
    if (bytes == 0) {
        return nullptr;
    }

    // Align the current pointer upward to the requested alignment.
    uintptr_t curr    = reinterpret_cast<uintptr_t>(current_);
    uintptr_t aligned = (curr + alignment - 1) & ~(alignment - 1);
    uintptr_t next    = aligned + bytes;

    if (next > reinterpret_cast<uintptr_t>(end_)) {
        return nullptr;  // Out of memory
    }

    current_ = reinterpret_cast<std::byte*>(next);
    return reinterpret_cast<void*>(aligned);
}

void BumpAllocator::reset() noexcept {
    current_ = base_;
}

size_t BumpAllocator::used() const noexcept {
    return static_cast<size_t>(current_ - base_);
}

size_t BumpAllocator::remaining() const noexcept {
    return static_cast<size_t>(end_ - current_);
}

size_t BumpAllocator::capacity() const noexcept {
    return capacity_;
}

} // namespace mem
