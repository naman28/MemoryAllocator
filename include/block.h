#pragma once
/// @file block.h
/// @brief Block header metadata for the custom memory allocator.
///
/// Memory layout per block:
///   [BlockHeader (64B)][Payload (N * 64B)]
///   ^--- always 64-byte aligned (cache-line aligned)
///
/// Two linked lists thread through blocks:
///   1. Physical list (next/prev): all blocks in heap address order
///   2. Free list (next_free/prev_free): only free blocks, for fast search

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <new>

namespace mem {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

/// Block alignment — all block boundaries are aligned to this value.
/// Supports alignment requests of 8, 16, 32, or 64 bytes.
inline constexpr size_t BLOCK_ALIGNMENT = 64;

/// Magic canary value written into every valid BlockHeader.
/// Used to detect metadata corruption (buffer overflows, use-after-free, etc.).
inline constexpr uint32_t BLOCK_MAGIC = 0xDEADBEEF;

/// Minimum payload size for any block (one alignment unit).
/// This also ensures there is room for FreeListNode-style metadata
/// if the block were to become free in the future.
inline constexpr size_t MIN_BLOCK_PAYLOAD = BLOCK_ALIGNMENT;

// ---------------------------------------------------------------------------
// Block flags
// ---------------------------------------------------------------------------

/// Bit flags stored in BlockHeader::flags.
enum BlockFlag : uint32_t {
    BLOCK_FLAG_NONE = 0,
    BLOCK_FLAG_FREE = 1u << 0,
};

// ---------------------------------------------------------------------------
// BlockHeader
// ---------------------------------------------------------------------------

/// Per-block metadata placed immediately before the payload.
///
/// Invariants:
///   - sizeof(BlockHeader) == BLOCK_ALIGNMENT (64 bytes, one cache line)
///   - Every BlockHeader starts at a BLOCK_ALIGNMENT-aligned address
///   - Every payload therefore also starts at a BLOCK_ALIGNMENT-aligned address
///   - `size` is always a multiple of BLOCK_ALIGNMENT
///   - `magic` == BLOCK_MAGIC when the header is valid
struct alignas(BLOCK_ALIGNMENT) BlockHeader {
    size_t       size;          ///< Payload size in bytes (always a multiple of BLOCK_ALIGNMENT)
    BlockHeader* next;          ///< Next block in physical (heap address) order; nullptr if last
    BlockHeader* prev;          ///< Previous block in physical order; nullptr if first
    BlockHeader* next_free;     ///< Next block in the free list; nullptr if not linked
    BlockHeader* prev_free;     ///< Previous block in the free list; nullptr if not linked
    uint32_t     magic;         ///< Canary value — must equal BLOCK_MAGIC
    uint32_t     flags;         ///< Block state bits (see BlockFlag)
    size_t       _reserved[2];  ///< Padding to fill the 64-byte cache line

    // ----- Accessors -----

    /// Returns true if this block is on the free list.
    [[nodiscard]] bool isFree() const noexcept {
        return (flags & BLOCK_FLAG_FREE) != 0;
    }

    /// Set or clear the free flag.
    void setFree(bool free) noexcept {
        if (free) flags |= BLOCK_FLAG_FREE;
        else      flags &= ~BLOCK_FLAG_FREE;
    }

    /// Pointer to the usable payload area (immediately after this header).
    [[nodiscard]] void* payload() noexcept {
        return reinterpret_cast<std::byte*>(this) + sizeof(BlockHeader);
    }
    [[nodiscard]] const void* payload() const noexcept {
        return reinterpret_cast<const std::byte*>(this) + sizeof(BlockHeader);
    }

    /// Pointer to the first byte past this block (header + payload).
    [[nodiscard]] std::byte* blockEnd() noexcept {
        return reinterpret_cast<std::byte*>(this) + sizeof(BlockHeader) + size;
    }
    [[nodiscard]] const std::byte* blockEnd() const noexcept {
        return reinterpret_cast<const std::byte*>(this) + sizeof(BlockHeader) + size;
    }

    /// Total footprint of this block (header + payload).
    [[nodiscard]] size_t totalSize() const noexcept {
        return sizeof(BlockHeader) + size;
    }

    /// Returns true if the canary is intact.
    [[nodiscard]] bool isValid() const noexcept {
        return magic == BLOCK_MAGIC;
    }

    /// Initialise all fields of a freshly-created header.
    void init(size_t payloadSize, bool isFreeBlock,
              BlockHeader* prevBlock = nullptr,
              BlockHeader* nextBlock = nullptr) noexcept {
        size       = payloadSize;
        next       = nextBlock;
        prev       = prevBlock;
        next_free  = nullptr;
        prev_free  = nullptr;
        magic      = BLOCK_MAGIC;
        flags      = isFreeBlock ? BLOCK_FLAG_FREE : BLOCK_FLAG_NONE;
        _reserved[0] = 0;
        _reserved[1] = 0;
    }
};

// ---------------------------------------------------------------------------
// Free-standing helpers
// ---------------------------------------------------------------------------

/// Recover the BlockHeader from a pointer previously returned by payload().
inline BlockHeader* headerFromPayload(void* payload) noexcept {
    return reinterpret_cast<BlockHeader*>(
        static_cast<std::byte*>(payload) - sizeof(BlockHeader));
}

/// Round `value` up to the nearest multiple of BLOCK_ALIGNMENT.
inline constexpr size_t alignUp(size_t value) noexcept {
    return (value + BLOCK_ALIGNMENT - 1) & ~(BLOCK_ALIGNMENT - 1);
}

/// Print a single block's metadata to `os` (for debugging / dumpHeap).
void printBlock(const BlockHeader* block, std::ostream& os);

// ---------------------------------------------------------------------------
// Compile-time sanity checks
// ---------------------------------------------------------------------------

static_assert(sizeof(BlockHeader) == BLOCK_ALIGNMENT,
              "BlockHeader must be exactly BLOCK_ALIGNMENT bytes (one cache line)");
static_assert(sizeof(BlockHeader) % BLOCK_ALIGNMENT == 0,
              "BlockHeader size must be a multiple of BLOCK_ALIGNMENT so payloads are aligned");
static_assert(alignof(BlockHeader) == BLOCK_ALIGNMENT,
              "BlockHeader alignment must match BLOCK_ALIGNMENT");

} // namespace mem
