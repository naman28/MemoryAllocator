#pragma once
/// @file freelist.h
/// @brief Segregated free-list manager for the custom allocator.
///
/// Free blocks are organized into 4 size classes (segregated free lists).
/// Each class maintains a doubly-linked list of free BlockHeaders via
/// the next_free / prev_free pointers.
///
/// Size classes:
///   [0] Tiny   : payload ≤  64 B
///   [1] Small  : payload ≤ 256 B
///   [2] Medium : payload ≤ 1024 B
///   [3] Large  : payload >  1024 B
///
/// On allocation, the search starts at the target class and falls through
/// to larger classes if the target is empty.  This gives O(1) best-case
/// for the most common allocation sizes.

#include "block.h"

#include <cstddef>
#include <climits>

namespace mem {

class FreeList {
public:
    static constexpr size_t NUM_SIZE_CLASSES = 4;
    static constexpr size_t TINY_MAX         = 64;
    static constexpr size_t SMALL_MAX        = 256;
    static constexpr size_t MEDIUM_MAX       = 1024;

    FreeList() = default;

    /// Insert a free block into the appropriate size-class list.
    /// @pre block->isFree() && block->isValid()
    void insert(BlockHeader* block);

    /// Remove a block from its size-class list.
    /// @pre The block is currently in a list (next_free / prev_free are valid).
    void remove(BlockHeader* block);

    /// First-fit: return the first block whose payload ≥ `size`,
    /// searching from the target size class upward.
    /// @return  Matching block, or nullptr if none is large enough.
    [[nodiscard]] BlockHeader* findFirst(size_t size);

    /// Best-fit: return the smallest block whose payload ≥ `size`.
    /// @return  Matching block, or nullptr if none is large enough.
    [[nodiscard]] BlockHeader* findBest(size_t size);

    /// True if every size-class list is empty.
    [[nodiscard]] bool empty() const noexcept;

    /// Total number of blocks across all size classes.
    [[nodiscard]] size_t count() const noexcept;

private:
    /// Map a payload size to its size-class index.
    [[nodiscard]] size_t sizeClass(size_t size) const noexcept;

    /// Head pointers for each size-class list.
    BlockHeader* heads_[NUM_SIZE_CLASSES] = {};
};

} // namespace mem
