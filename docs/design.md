# Allocator Design Document

## Overview

This document describes the internal design of the custom memory allocator.

## Memory Layout

The allocator reserves a single contiguous region of memory via `std::aligned_alloc`.
All blocks are laid out sequentially within this region:

```
┌──────────────────────────────────────────────────────────┐
│ Heap Region (capacity bytes, 64-byte aligned)            │
│                                                          │
│ ┌────────────┬──────────┬────────────┬──────────┬───┐   │
│ │  Header 0  │ Payload 0│  Header 1  │ Payload 1│...│   │
│ │  (64 B)    │ (N×64 B) │  (64 B)    │ (M×64 B) │   │   │
│ └────────────┴──────────┴────────────┴──────────┴───┘   │
│ ← Block 0 ────────────→ ← Block 1 ────────────→         │
└──────────────────────────────────────────────────────────┘
```

## Block Header

Each block has a 64-byte header (one cache line):

```
Offset  Size  Field       Description
──────  ────  ─────       ───────────
 0       8    size        Payload size in bytes (multiple of 64)
 8       8    next        Pointer to next physical block (or nullptr)
16       8    prev        Pointer to previous physical block (or nullptr)
24       8    next_free   Pointer to next free block in free list
32       8    prev_free   Pointer to previous free block in free list
40       4    magic       Canary value (0xDEADBEEF)
44       4    flags       Bit 0 = is_free
48      16    _reserved   Padding to 64 bytes
```

**Why 64 bytes?**
- Exactly one cache line on ARM64 — no false sharing
- Divisible by all supported alignments (8, 16, 32, 64)
- Payload immediately follows at a 64-byte aligned address

## Allocation Algorithm

```
allocate(bytes):
    alignedSize ← roundUp(bytes, 64)
    alignedSize ← max(alignedSize, 64)        // minimum payload

    block ← freeList.findFirst(alignedSize)    // segregated first-fit
    if block == null: return null               // OOM

    freeList.remove(block)

    if block.size - alignedSize ≥ 128:         // 64 header + 64 min payload
        splitBlock(block, alignedSize)

    block.setFree(false)
    return block.payload()
```

## Deallocation Algorithm

```
deallocate(ptr):
    if ptr == null: return
    validate(ptr)                               // check ownership, canary

    block ← headerFromPayload(ptr)
    assert(block.magic == 0xDEADBEEF)
    assert(!block.isFree())                     // double-free check

    block.setFree(true)
    block ← coalesce(block)                     // merge with neighbours
    freeList.insert(block)
```

## Block Splitting

When a free block is larger than needed, we split it:

```
Before split (1024 B payload):
┌────────────┬─────────────────────────────────────┐
│  Header    │  Payload (1024 B)                    │
└────────────┴─────────────────────────────────────┘

After split (requested 128 B):
┌────────────┬──────────┬────────────┬─────────────┐
│  Header    │ 128 B    │  Header    │ 832 B       │
│  (used)    │ (used)   │  (free)    │ (free)      │
└────────────┴──────────┴────────────┴─────────────┘
```

Minimum split: remainder must be ≥ 128 bytes (64 header + 64 min payload).

## Block Coalescing

On deallocation, we check both physical neighbours:

```
Case 1 — Merge with next:
[FREE A] [FREE B] → [FREE A+B+header]

Case 2 — Merge with prev:
[FREE A] [FREE B] → [FREE A+B+header] (A absorbs B)

Case 3 — Triple merge:
[FREE A] [FREE B] [FREE C] → [FREE A+B+C+2*header]
```

This runs in O(1) since we have prev/next pointers in each header.

## Segregated Free Lists

Free blocks are organized into 4 size classes:

```
Class 0 (Tiny):   payload ≤   64 B
Class 1 (Small):  payload ≤  256 B
Class 2 (Medium): payload ≤ 1024 B
Class 3 (Large):  payload > 1024 B
```

Each class has its own doubly-linked list of free blocks.
On allocation, we search from the target class upward.
This gives O(1) best-case for common sizes (most allocations are small).

## Alignment Strategy

All blocks are 64-byte aligned.  Since `sizeof(BlockHeader) = 64` and
payloads are rounded to multiples of 64, every payload address is:

```
payload_addr = header_addr + 64
payload_addr % 64 == 0   (since header_addr % 64 == 0)
```

This natively supports alignment requests of 8, 16, 32, and 64 bytes
with zero padding overhead.

**Tradeoff**: minimum allocation is 64 bytes of payload (+ 64 bytes header
= 128 bytes total). This means small allocations (e.g. 1 byte) waste up
to 63 bytes.  For workloads dominated by tiny allocations, a pool/slab
allocator is recommended (stretch goal).

## Complexity Analysis

| Operation    | Time       | Space    |
|-------------|------------|----------|
| allocate    | O(n) worst, O(1) typical* | O(1) per block |
| deallocate  | O(1)       | O(1)     |
| reallocate  | O(n) worst | O(1)     |
| coalesce    | O(1)       | O(1)     |
| split       | O(1)       | O(1)     |
| getStats    | O(n)       | O(1)     |

*With segregated lists, small allocations (≤64B) are typically O(1) since
they check a dedicated list first.

## Error Detection

| Error             | Detection Method              |
|-------------------|-------------------------------|
| Corrupted header  | Canary (magic ≠ 0xDEADBEEF)   |
| Double free       | `isFree()` check before free  |
| Invalid pointer   | Heap bounds check             |
| Null pointer      | Explicit null check (no-op)   |
| Misaligned pointer| Caught by canary mismatch     |

## Future Improvements

- **Pool allocator** for fixed-size small objects
- **Buddy allocator** for power-of-2 allocations
- **Thread-local caches** for lock-free multi-threaded allocation
- **Huge pages** (mmap with MAP_HUGETLB) for large heaps
- **NUMA-aware allocation** for multi-socket systems
- **Guard pages** for buffer overflow detection
- **Allocation tracing** for leak detection
