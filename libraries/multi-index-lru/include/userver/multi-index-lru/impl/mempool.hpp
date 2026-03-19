#pragma once

#include <memory>
#include <cstddef>
#include <cassert>
#include <iostream>

USERVER_NAMESPACE_BEGIN

namespace multi_index_lru {

static std::size_t align_up(std::size_t size) noexcept {
    constexpr std::size_t alignment = alignof(std::max_align_t);
    return (size + alignment - 1) & ~(alignment - 1);
}

class FixedPool {
public:
    explicit FixedPool(std::size_t capacity, std::size_t elem_size)
        : capacity_(capacity)
        , element_size_(align_up(elem_size))
        , storage_(nullptr)
        , free_head_(nullptr) {

        if (capacity_ == 0) {
            return;
        }
        
        std::size_t total_size = capacity_ * element_size_;
        
        storage_ = static_cast<char*>(::operator new(total_size));
        
        char* current = storage_;
        free_head_ = nullptr;
        
        for (std::size_t i = 0; i < capacity_; ++i) {
            
            void** next_ptr = reinterpret_cast<void**>(current);
            *next_ptr = free_head_;
            free_head_ = current;
            
            current += element_size_;
        }
    }

    ~FixedPool() {
        ::operator delete(storage_);
    }

    FixedPool(const FixedPool&) = delete;
    FixedPool& operator=(const FixedPool&) = delete;

    void* allocate() {
        if (!free_head_) {
            std::cerr << "ERROR: FixedPool out of memory! capacity=" << capacity_ << std::endl;
            throw std::bad_alloc();
        }
        
        void* block = free_head_;
        
        void** next_ptr = static_cast<void**>(block);
        free_head_ = *next_ptr;
        
        return block;
    }

    void deallocate(void* ptr) noexcept {
        if (!ptr) {
            return;
        }
        
        char* char_ptr = static_cast<char*>(ptr);
        char* storage_end = storage_ + capacity_ * element_size_;
        
        if (char_ptr < storage_ || char_ptr >= storage_end) {
            std::cerr << "ERROR: FixedPool deallocating pointer not from pool!" << std::endl;
            std::cerr << "  ptr: " << ptr << " is outside [" 
                      << static_cast<void*>(storage_) << ", " 
                      << static_cast<void*>(storage_end) << ")" << std::endl;
            return;
        }
        
        void** next_ptr = static_cast<void**>(ptr);
        *next_ptr = free_head_;
        free_head_ = ptr;
    }

    std::size_t capacity() const noexcept { return capacity_; }
    std::size_t element_size() const noexcept { return element_size_; }

private:
    std::size_t capacity_;
    std::size_t element_size_;
    char* storage_;
    void* free_head_;
};

template <typename T>
class FixedPoolAllocator {
public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::false_type;

    FixedPoolAllocator() noexcept 
        : pool_(nullptr) {}

    explicit FixedPoolAllocator(std::size_t capacity) noexcept
        : FixedPoolAllocator(capacity, sizeof(T)) {}

    FixedPoolAllocator(std::size_t capacity, std::size_t element_size) noexcept
        : pool_(std::make_shared<FixedPool>(capacity, element_size)) {}

    FixedPoolAllocator(const FixedPoolAllocator& other) = delete;

    FixedPoolAllocator(FixedPoolAllocator&& other) = delete;

    template <typename U>
    FixedPoolAllocator(const FixedPoolAllocator<U>& other) noexcept {
        pool_ = std::make_shared<FixedPool>(
            other.pool_->capacity(),
            sizeof(T) 
        );
    }

    ~FixedPoolAllocator() {}

    T* allocate(size_type n) {
        if (n != 1) {
            return static_cast<T*>(::operator new(n * sizeof(T)));
        }
        
        if (!pool_) {
            std::cerr << "  ERROR: pool_ is null!" << std::endl;
            throw std::bad_alloc();
        }
        
        void* ptr = pool_->allocate();
        
        T* result = static_cast<T*>(ptr);
        
        return result;
    }

    void deallocate(T* ptr, size_type n) noexcept {
        
        if (n != 1) {
            ::operator delete(ptr);
            return;
        }
        
        if (pool_) {
            pool_->deallocate(ptr);
        } 
    }

    template <typename U> struct rebind { using other = FixedPoolAllocator<U>; };

    template <typename U>
    bool operator==(const FixedPoolAllocator<U>& other) const noexcept {
        bool eq = (pool_ == other.pool_);
        return eq;
    }

    template <typename U>
    bool operator!=(const FixedPoolAllocator<U>& other) const noexcept {
        return !(*this == other);
    }

    std::size_t get_element_size() const noexcept {
        return pool_ ? pool_->element_size() : 0;
    }

private:
    template <typename U> friend class FixedPoolAllocator;
    
    std::shared_ptr<FixedPool> pool_;
};

}  // namespace multi_index_lru

USERVER_NAMESPACE_END