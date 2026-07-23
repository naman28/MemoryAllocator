/// @file allocator.cpp
/// @brief HeapAllocator — full implementation.
///
/// Implements: allocate, deallocate, reallocate, split, coalesce,
///             statistics, and heap dump.

#include "allocator.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace mem {

// ===========================================================================
// Construction
// ===========================================================================

HeapAllocator::HeapAllocator(size_t capacity)
    : heap_(capacity)
{
    // The entire heap begins as a single large free block.
    //
    //  [  BlockHeader (64 B)  ][ ........  payload  ........ ]
    //  ^--- heap_.base()
    //
    // payload size = capacity - sizeof(BlockHeader), rounded down to alignment.

    size_t payloadSize = heap_.capacity() - sizeof(BlockHeader);
    payloadSize        = (payloadSize / BLOCK_ALIGNMENT) * BLOCK_ALIGNMENT;

    firstBlock_ = reinterpret_cast<BlockHeader*>(heap_.base());
    firstBlock_->init(payloadSize, /*isFreeBlock=*/true);
    freeList_.insert(firstBlock_);
}

// ===========================================================================
// allocate
// ===========================================================================

void* HeapAllocator::allocate(size_t bytes) {
    return allocate(bytes, BLOCK_ALIGNMENT);
}

void* HeapAllocator::allocate(size_t bytes, size_t alignment) {
    if (bytes == 0) return nullptr;

    // Validate alignment (must be power of 2, ≤ BLOCK_ALIGNMENT).
    if (alignment == 0
        || (alignment & (alignment - 1)) != 0
        || alignment > BLOCK_ALIGNMENT) {
        return nullptr;
    }

    // Round requested size up to a multiple of BLOCK_ALIGNMENT.
    size_t alignedSize = alignUp(bytes);
    if (alignedSize < MIN_BLOCK_PAYLOAD) {
        alignedSize = MIN_BLOCK_PAYLOAD;
    }

    // Search the segregated free list (first-fit, cascading upward).
    BlockHeader* block = freeList_.findFirst(alignedSize);
    if (!block) [[unlikely]] {
        return nullptr;  // Out of memory
    }

    // Remove the chosen block from the free list.
    freeList_.remove(block);

    // Split if the remainder is large enough for a new block.
    splitBlock(block, alignedSize);

    // Mark as allocated.
    block->setFree(false);
    ++allocationCount_;

    return block->payload();
}

// ===========================================================================
// deallocate
// ===========================================================================

void HeapAllocator::deallocate(void* ptr) {
    if (!ptr) return;  // Freeing nullptr is a no-op (like C free / C++ delete).

    // --- Error detection ---

    if (!isOwnedPointer(ptr)) {
        throw std::invalid_argument(
            "deallocate: pointer is not owned by this allocator");
    }

    BlockHeader* block = headerFromPayload(ptr);

    if (!block->isValid()) {
        throw std::runtime_error(
            "deallocate: corrupted block metadata (bad canary)");
    }

    if (block->isFree()) {
        throw std::runtime_error(
            "deallocate: double free detected");
    }

    // --- Free the block ---

    block->setFree(true);
    ++deallocationCount_;

    // Coalesce with physically-adjacent free neighbours.
    // `coalesce` returns the (possibly enlarged) resulting block.
    block = coalesce(block);

    // Insert the (merged) block into the free list.
    freeList_.insert(block);
}

// ===========================================================================
// reallocate
// ===========================================================================

void* HeapAllocator::reallocate(void* ptr, size_t newSize) {
    // reallocate(nullptr, n) ≡ allocate(n)
    if (!ptr) return allocate(newSize);

    // reallocate(ptr, 0) ≡ deallocate(ptr)
    if (newSize == 0) {
        deallocate(ptr);
        return nullptr;
    }

    // --- Validate ---

    if (!isOwnedPointer(ptr)) {
        throw std::invalid_argument(
            "reallocate: pointer is not owned by this allocator");
    }

    BlockHeader* block = headerFromPayload(ptr);

    if (!block->isValid()) {
        throw std::runtime_error("reallocate: corrupted block metadata");
    }
    if (block->isFree()) {
        throw std::runtime_error("reallocate: block is already free");
    }

    size_t alignedNewSize = alignUp(newSize);
    if (alignedNewSize < MIN_BLOCK_PAYLOAD) {
        alignedNewSize = MIN_BLOCK_PAYLOAD;
    }

    const size_t currentSize = block->size;

    // ----- Case 1: Shrink in-place -----
    if (alignedNewSize <= currentSize) {
        // Try to split off the excess as a new free block.
        const size_t excess = currentSize - alignedNewSize;
        if (excess >= sizeof(BlockHeader) + MIN_BLOCK_PAYLOAD) {
            auto* remainder = reinterpret_cast<BlockHeader*>(
                static_cast<std::byte*>(block->payload()) + alignedNewSize);

            remainder->init(
                excess - sizeof(BlockHeader),
                /*isFreeBlock=*/true,
                /*prev=*/block,
                /*next=*/block->next);

            if (block->next) block->next->prev = remainder;
            block->next = remainder;
            block->size = alignedNewSize;

            // Coalesce the remainder with its next neighbour if also free.
            remainder = coalesce(remainder);
            freeList_.insert(remainder);
        }
        // else: excess too small — keep the full block (internal fragmentation).
        return ptr;
    }

    // ----- Case 2: Expand in-place (merge with next free block) -----
    if (block->next && block->next->isFree()) {
        const size_t combined =
            currentSize + sizeof(BlockHeader) + block->next->size;

        if (combined >= alignedNewSize) {
            // Absorb the next block.
            freeList_.remove(block->next);

            BlockHeader* nextNext = block->next->next;
            block->next->magic = 0;  // Invalidate absorbed header.
            block->size = combined;
            block->next = nextNext;
            if (nextNext) nextNext->prev = block;

            // Split off any excess from the expanded block.
            splitBlock(block, alignedNewSize);

            return ptr;
        }
    }

    // ----- Case 3: Allocate + copy + free -----
    void* newPtr = allocate(newSize);
    if (!newPtr) return nullptr;

    std::memcpy(newPtr, ptr, std::min(currentSize, alignedNewSize));
    deallocate(ptr);

    return newPtr;
}

// ===========================================================================
// splitBlock
// ===========================================================================

void HeapAllocator::splitBlock(BlockHeader* block, size_t requestedSize) {
    const size_t remaining = block->size - requestedSize;

    // Only split if the remainder can hold a full block (header + min payload).
    if (remaining < sizeof(BlockHeader) + MIN_BLOCK_PAYLOAD) {
        return;  // Give the whole block to the caller — slight internal fragmentation.
    }

    // Create a new free block in the excess space.
    auto* newBlock = reinterpret_cast<BlockHeader*>(
        static_cast<std::byte*>(block->payload()) + requestedSize);

    newBlock->init(
        remaining - sizeof(BlockHeader),
        /*isFreeBlock=*/true,
        /*prev=*/block,
        /*next=*/block->next);

    // Patch the physical linked list.
    if (block->next) {
        block->next->prev = newBlock;
    }
    block->next = newBlock;
    block->size = requestedSize;

    // Add the new free block to the free list.
    freeList_.insert(newBlock);
}

// ===========================================================================
// coalesce
// ===========================================================================

BlockHeader* HeapAllocator::coalesce(BlockHeader* block) {
    // --- Merge with the NEXT physical block (if free) ---
    if (block->next && block->next->isFree()) {
        BlockHeader* nextBlock = block->next;
        freeList_.remove(nextBlock);

        block->size += sizeof(BlockHeader) + nextBlock->size;
        block->next  = nextBlock->next;
        if (nextBlock->next) {
            nextBlock->next->prev = block;
        }

        nextBlock->magic = 0;  // Invalidate absorbed header.
    }

    // --- Merge with the PREVIOUS physical block (if free) ---
    if (block->prev && block->prev->isFree()) {
        BlockHeader* prevBlock = block->prev;
        freeList_.remove(prevBlock);

        prevBlock->size += sizeof(BlockHeader) + block->size;
        prevBlock->next  = block->next;
        if (block->next) {
            block->next->prev = prevBlock;
        }

        block->magic = 0;  // Invalidate absorbed header.
        block = prevBlock;  // The merged block is now `prevBlock`.
    }

    return block;
}

// ===========================================================================
// Statistics
// ===========================================================================

size_t HeapAllocator::allocatedMemory() const noexcept {
    size_t total = 0;
    for (const BlockHeader* b = firstBlock_; b; b = b->next) {
        if (!b->isFree()) total += b->size;
    }
    return total;
}

size_t HeapAllocator::freeMemory() const noexcept {
    size_t total = 0;
    for (const BlockHeader* b = firstBlock_; b; b = b->next) {
        if (b->isFree()) total += b->size;
    }
    return total;
}

HeapStats HeapAllocator::getStats() const {
    HeapStats s;
    s.total_capacity     = heap_.capacity();
    s.allocation_count   = allocationCount_;
    s.deallocation_count = deallocationCount_;

    for (const BlockHeader* b = firstBlock_; b; b = b->next) {
        ++s.total_blocks;
        s.overhead_bytes += sizeof(BlockHeader);

        if (b->isFree()) {
            s.free_bytes += b->size;
            ++s.free_blocks;
            if (b->size > s.largest_free_block) {
                s.largest_free_block = b->size;
            }
        } else {
            s.allocated_bytes += b->size;
        }
    }

    if (s.free_bytes > 0) {
        s.fragmentation_ratio =
            1.0 - static_cast<double>(s.largest_free_block)
                 / static_cast<double>(s.free_bytes);
    }

    return s;
}

// ===========================================================================
// dumpHeap
// ===========================================================================

void HeapAllocator::dumpHeap(std::ostream& os) const {
    os << "\n=== Heap Dump ===\n"
       << "Capacity : " << heap_.capacity() << " bytes\n"
       << "Base     : " << static_cast<const void*>(heap_.base()) << "\n\n";

    size_t idx = 0;
    for (const BlockHeader* b = firstBlock_; b; b = b->next, ++idx) {
        os << "  Block #" << idx << ":  ";
        printBlock(b, os);
    }

    os << "\n";
    HeapStats stats = getStats();
    printStats(stats, os);
    os << "=================\n\n";
}

// ===========================================================================
// Helpers
// ===========================================================================

bool HeapAllocator::isOwnedPointer(const void* ptr) const noexcept {
    return heap_.contains(ptr);
}

} // namespace mem
