/// @file freelist.cpp
/// @brief Segregated free-list implementation.

#include "freelist.h"

namespace mem {

// ---------------------------------------------------------------------------
// Size-class mapping
// ---------------------------------------------------------------------------

size_t FreeList::sizeClass(size_t size) const noexcept {
    if (size <= TINY_MAX)   return 0;
    if (size <= SMALL_MAX)  return 1;
    if (size <= MEDIUM_MAX) return 2;
    return 3;
}

// ---------------------------------------------------------------------------
// Insert / Remove
// ---------------------------------------------------------------------------

void FreeList::insert(BlockHeader* block) {
    const size_t cls = sizeClass(block->size);

    // Prepend to the head of the target size-class list.
    block->next_free = heads_[cls];
    block->prev_free = nullptr;

    if (heads_[cls]) {
        heads_[cls]->prev_free = block;
    }
    heads_[cls] = block;
}

void FreeList::remove(BlockHeader* block) {
    const size_t cls = sizeClass(block->size);

    if (block->prev_free) {
        block->prev_free->next_free = block->next_free;
    } else {
        // block was the head of this class
        heads_[cls] = block->next_free;
    }

    if (block->next_free) {
        block->next_free->prev_free = block->prev_free;
    }

    block->next_free = nullptr;
    block->prev_free = nullptr;
}

// ---------------------------------------------------------------------------
// Search
// ---------------------------------------------------------------------------

BlockHeader* FreeList::findFirst(size_t size) {
    const size_t cls = sizeClass(size);

    // Search from the target class upward (small → large).
    for (size_t c = cls; c < NUM_SIZE_CLASSES; ++c) {
        BlockHeader* curr = heads_[c];
        while (curr) {
            if (curr->size >= size) {
                return curr;
            }
            curr = curr->next_free;
        }
    }
    return nullptr;
}

BlockHeader* FreeList::findBest(size_t size) {
    BlockHeader* best     = nullptr;
    size_t       bestSize = SIZE_MAX;

    const size_t cls = sizeClass(size);

    for (size_t c = cls; c < NUM_SIZE_CLASSES; ++c) {
        BlockHeader* curr = heads_[c];
        while (curr) {
            if (curr->size >= size && curr->size < bestSize) {
                best     = curr;
                bestSize = curr->size;
                if (bestSize == size) return best;  // Perfect fit — stop early.
            }
            curr = curr->next_free;
        }
    }
    return best;
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

bool FreeList::empty() const noexcept {
    for (size_t c = 0; c < NUM_SIZE_CLASSES; ++c) {
        if (heads_[c]) return false;
    }
    return true;
}

size_t FreeList::count() const noexcept {
    size_t n = 0;
    for (size_t c = 0; c < NUM_SIZE_CLASSES; ++c) {
        BlockHeader* curr = heads_[c];
        while (curr) {
            ++n;
            curr = curr->next_free;
        }
    }
    return n;
}

} // namespace mem
