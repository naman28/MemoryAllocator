/// @file heap.cpp
/// @brief Heap — raw memory region management.

#include "heap.h"

#include <cstdlib>
#include <stdexcept>
#include <string>

namespace mem {

Heap::Heap(size_t capacity) {
    // Round capacity up to BLOCK_ALIGNMENT
    capacity_ = alignUp(capacity);

    // Minimum: one header + one minimum-payload block
    if (capacity_ < sizeof(BlockHeader) + MIN_BLOCK_PAYLOAD) {
        throw std::invalid_argument(
            "Heap capacity too small (need at least "
            + std::to_string(sizeof(BlockHeader) + MIN_BLOCK_PAYLOAD) + " bytes)");
    }

    // Allocate aligned memory (BLOCK_ALIGNMENT = 64)
    base_ = static_cast<std::byte*>(std::aligned_alloc(BLOCK_ALIGNMENT, capacity_));
    if (!base_) {
        throw std::bad_alloc();
    }
}

Heap::~Heap() {
    std::free(base_);
}

bool Heap::contains(const void* ptr) const noexcept {
    auto p = static_cast<const std::byte*>(ptr);
    return p >= base_ && p < base_ + capacity_;
}

} // namespace mem
