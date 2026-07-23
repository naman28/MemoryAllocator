#pragma once
/// @file stats.h
/// @brief Heap statistics for the custom memory allocator.

#include <cstddef>
#include <iosfwd>

namespace mem {

/// Snapshot of the allocator's current memory state.
struct HeapStats {
    size_t total_capacity      = 0;   ///< Total heap capacity in bytes
    size_t allocated_bytes     = 0;   ///< Sum of payload sizes for allocated blocks
    size_t free_bytes          = 0;   ///< Sum of payload sizes for free blocks
    size_t overhead_bytes      = 0;   ///< Total bytes consumed by BlockHeaders
    size_t largest_free_block  = 0;   ///< Payload size of the largest free block
    size_t allocation_count    = 0;   ///< Cumulative number of allocate() calls
    size_t deallocation_count  = 0;   ///< Cumulative number of deallocate() calls
    size_t total_blocks        = 0;   ///< Current number of blocks (free + allocated)
    size_t free_blocks         = 0;   ///< Current number of free blocks
    double fragmentation_ratio = 0.0; ///< 1 − (largest_free / total_free); 0 = no fragmentation
};

/// Pretty-print a HeapStats snapshot.
void printStats(const HeapStats& stats, std::ostream& os);

} // namespace mem
