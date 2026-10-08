// Replaces the global allocation functions with counting versions so that a test can assert that
// a region of code does not allocate at all, whatever allocator it would have used.

#include "allocation_counter.hpp"

#include <atomic>
#include <cstdlib>
#include <new>

namespace {

std::atomic<std::size_t> allocations{0};

void* Allocate(std::size_t size) {
    allocations.fetch_add(1, std::memory_order_relaxed);
    void* p = std::malloc(size == 0 ? 1 : size);
    if(p == nullptr) {
        std::abort();
    }
    return p;
}

void* AllocateAligned(std::size_t size, std::size_t alignment) {
    allocations.fetch_add(1, std::memory_order_relaxed);
    void* p = nullptr;
    if(posix_memalign(&p, alignment < sizeof(void*) ? sizeof(void*) : alignment, size == 0 ? alignment : size) != 0) {
        std::abort();
    }
    return p;
}

} // namespace

namespace eerie_leap::expression_engine::testing {

std::size_t GlobalAllocations() noexcept {
    return allocations.load(std::memory_order_relaxed);
}

} // namespace eerie_leap::expression_engine::testing

void* operator new(std::size_t size) {
    return Allocate(size);
}

void* operator new[](std::size_t size) {
    return Allocate(size);
}

void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
    return Allocate(size);
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
    return Allocate(size);
}

void* operator new(std::size_t size, std::align_val_t alignment) {
    return AllocateAligned(size, static_cast<std::size_t>(alignment));
}

void* operator new[](std::size_t size, std::align_val_t alignment) {
    return AllocateAligned(size, static_cast<std::size_t>(alignment));
}

void operator delete(void* p) noexcept {
    std::free(p);
}

void operator delete[](void* p) noexcept {
    std::free(p);
}

void operator delete(void* p, std::size_t) noexcept {
    std::free(p);
}

void operator delete[](void* p, std::size_t) noexcept {
    std::free(p);
}

void operator delete(void* p, std::align_val_t) noexcept {
    std::free(p);
}

void operator delete[](void* p, std::align_val_t) noexcept {
    std::free(p);
}

void operator delete(void* p, std::size_t, std::align_val_t) noexcept {
    std::free(p);
}

void operator delete[](void* p, std::size_t, std::align_val_t) noexcept {
    std::free(p);
}
