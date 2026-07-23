/// @file block.cpp
/// @brief BlockHeader printing utility.

#include "block.h"

#include <iomanip>
#include <iostream>

namespace mem {

void printBlock(const BlockHeader* block, std::ostream& os) {
    os << "["
       << (block->isFree() ? "FREE " : "USED ")
       << std::setw(8) << block->size << "B]"
       << "  @" << static_cast<const void*>(block)
       << "  payload:" << block->payload()
       << "  magic:0x" << std::hex << block->magic << std::dec;
    if (!block->isValid()) {
        os << "  *** CORRUPTED ***";
    }
    os << "\n";
}

} // namespace mem
