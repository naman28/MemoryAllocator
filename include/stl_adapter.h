#pragma once
/// @file stl_adapter.h
/// @brief STL-compatible allocator adapter wrapping HeapAllocator.
///
/// Usage:
///   mem::HeapAllocator heap(1048576);
///   mem::StlAllocator<int> alloc(heap);
///   std::vector<int, mem::StlAllocator<int>> vec(alloc);
///   vec.push_back(42);

#include "allocator.h"

#include <cstddef>
#include <limits>
#include <new>
#include <type_traits>

namespace mem {

/// A stateful allocator that satisfies the C++ named requirement *Allocator*.
/// It forwards all allocation/deallocation to a HeapAllocator instance.
template <typename T>
class StlAllocator {
public:
    // ---- Required type aliases ----
    using value_type      = T;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal                        = std::false_type;

    // ---- Construction ----

    /// Bind this adapter to an existing HeapAllocator.
    explicit StlAllocator(HeapAllocator& alloc) noexcept : alloc_(&alloc) {}

    /// Rebind copy constructor (required for containers that allocate
    /// internal node types different from T, e.g. std::map).
    template <typename U>
    StlAllocator(const StlAllocator<U>& other) noexcept
        : alloc_(other.allocator()) {}

    // ---- Allocate / Deallocate ----

    /// Allocate space for `n` objects of type T.
    /// @throws std::bad_alloc on failure.
    [[nodiscard]] T* allocate(std::size_t n) {
        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
            throw std::bad_alloc();
        }
        void* ptr = alloc_->allocate(n * sizeof(T));
        if (!ptr) {
            throw std::bad_alloc();
        }
        return static_cast<T*>(ptr);
    }

    /// Deallocate space previously allocated via allocate().
    void deallocate(T* p, [[maybe_unused]] std::size_t n) noexcept {
        alloc_->deallocate(p);
    }

    // ---- Accessors ----

    /// Return the underlying HeapAllocator (used by rebind constructor).
    [[nodiscard]] HeapAllocator* allocator() const noexcept { return alloc_; }

    // ---- Equality ----

    template <typename U>
    bool operator==(const StlAllocator<U>& other) const noexcept {
        return alloc_ == other.allocator();
    }

    template <typename U>
    bool operator!=(const StlAllocator<U>& other) const noexcept {
        return !(*this == other);
    }

private:
    HeapAllocator* alloc_;
};

} // namespace mem
