/// @file stats.cpp
/// @brief HeapStats pretty-printing.

#include "stats.h"

#include <iomanip>
#include <iostream>

namespace mem {

void printStats(const HeapStats& stats, std::ostream& os) {
    os << "--- Heap Statistics ---\n"
       << "  Total Capacity:       " << std::setw(10) << stats.total_capacity      << " bytes\n"
       << "  Allocated:            " << std::setw(10) << stats.allocated_bytes     << " bytes\n"
       << "  Free:                 " << std::setw(10) << stats.free_bytes          << " bytes\n"
       << "  Overhead (headers):   " << std::setw(10) << stats.overhead_bytes      << " bytes\n"
       << "  Largest Free Block:   " << std::setw(10) << stats.largest_free_block  << " bytes\n"
       << "  Total Blocks:         " << std::setw(10) << stats.total_blocks        << "\n"
       << "  Free Blocks:          " << std::setw(10) << stats.free_blocks         << "\n"
       << "  Allocation Count:     " << std::setw(10) << stats.allocation_count    << "\n"
       << "  Deallocation Count:   " << std::setw(10) << stats.deallocation_count  << "\n"
       << "  Fragmentation:        " << std::setw(9)  << std::fixed << std::setprecision(2)
       << (stats.fragmentation_ratio * 100.0) << "%\n"
       << "-----------------------\n";
}

} // namespace mem
