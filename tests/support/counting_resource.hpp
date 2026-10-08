#pragma once

#include <cstddef>
#include <cstdlib>
#include <memory_resource>
#include <new>

namespace eerie_leap::expression_engine::testing {

// Counts the allocations made through it; optionally fails every allocation.
class CountingResource final : public std::pmr::memory_resource {
public:
    explicit CountingResource(bool fail = false) noexcept : fail_(fail) {}

    [[nodiscard]] std::size_t Allocations() const noexcept {
        return allocations_;
    }

    [[nodiscard]] std::size_t Deallocations() const noexcept {
        return deallocations_;
    }

    [[nodiscard]] std::size_t BytesAllocated() const noexcept {
        return bytes_;
    }

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        if(fail_) {
#if defined(__cpp_exceptions)
            throw std::bad_alloc();
#else
            std::abort();
#endif
        }
        ++allocations_;
        bytes_ += bytes;
        return std::pmr::new_delete_resource()->allocate(bytes, alignment);
    }

    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override {
        ++deallocations_;
        std::pmr::new_delete_resource()->deallocate(p, bytes, alignment);
    }

    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }

    bool fail_;
    std::size_t allocations_ = 0;
    std::size_t deallocations_ = 0;
    std::size_t bytes_ = 0;
};

} // namespace eerie_leap::expression_engine::testing
