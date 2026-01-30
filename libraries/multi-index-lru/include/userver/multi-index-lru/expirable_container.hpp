#pragma once

/// @file userver/multi-index-lru/container.hpp
/// @brief @copybrief multi_index_lru::ExpirableContainer

#include <mutex>
#include <thread>
#include <functional>
#include <atomic>
#include <cassert> 
#include <array>

#include "impl/mpl_helpers.hpp"
#define _USERVER_MULTIINDEX_LRU_MUTEX_COUNT 100

USERVER_NAMESPACE_BEGIN

namespace multi_index_lru {

/// @ingroup userver_containers
///
/// @brief MultiIndex LRU expirable container
template <typename Value, typename IndexSpecifierList, typename Allocator = std::allocator<Value>>
class ExpirableContainer {
public:
    explicit ExpirableContainer(size_t max_size,
                       std::chrono::milliseconds ttl,
                       std::chrono::milliseconds cleanup_interval = std::chrono::milliseconds(60))
        : max_size_(max_size), ttl_(ttl), cleanup_interval_(cleanup_interval), cleanup_thread_running_(false)
    {
        assert(ttl.count() > 0 && "ttl must be positive");
        assert(cleanup_interval.count() > 0 && "cleanup_interval must be positive");
    }

    ~ExpirableContainer() {
        stop_cleanup();
    }

    template <typename... Args>
    bool emplace(Args&&... args) {

        lock_all_mutexes();

        auto& seq_index = container_.template get<0>();
        auto result = seq_index.emplace_front(std::forward<Args>(args)...);

        if (!result.second) {
            seq_index.relocate(seq_index.begin(), result.first);
            seq_index.modify(result.first, [](CacheItem& item) {
                item.last_accessed = std::chrono::steady_clock::now();
            });
        } else if (seq_index.size() > max_size_) {
            seq_index.pop_back();
        }

        unlock_all_mutexes();

        if (!cleanup_thread_running_.load(std::memory_order_relaxed)) {
            start_cleanup();
        }


        return result.second;
    }

    bool insert(const Value& value) { return emplace(value); }

    bool insert(Value&& value) { return emplace(std::move(value)); }

    template <typename Tag, typename Key>
    auto find(const Key& key) {
        auto& primary_index = container_.template get<Tag>();
        auto it = primary_index.find(key);

        std::size_t element_ptr = (std::size_t) it.operator->();
        std::lock_guard<std::mutex> lock(mutexes_[element_ptr % _USERVER_MULTIINDEX_LRU_MUTEX_COUNT]);

        it = primary_index.find(key);

        if (it != primary_index.end()) {
            if (std::chrono::steady_clock::now() > it->last_accessed + ttl_) {
                primary_index.erase(it);
                return impl::TimestampedIteratorWrapper{primary_index.end()};
            }

            auto& seq_index = container_.template get<0>();
            auto seq_it = container_.template project<0>(it);
            seq_index.relocate(seq_index.begin(), seq_it);

            primary_index.modify(it, [](CacheItem& item) {
                item.last_accessed = std::chrono::steady_clock::now();
            });
        }

        return impl::TimestampedIteratorWrapper{it};
    }

    template <typename Tag, typename Key>
    bool contains(const Key& key) {
        return this->template find<Tag, Key>(key) != container_.template get<Tag>().end();
    }

    template <typename Tag, typename Key>
    bool erase(const Key& key) {
        auto& primary_index = container_.template get<Tag>();
        auto it = primary_index.find(key);

        if (it == primary_index.end()) {
            return false;
        }

        std::size_t element_ptr = (std::size_t) it.operator->();
        std::lock_guard<std::mutex> lock(mutexes_[element_ptr % _USERVER_MULTIINDEX_LRU_MUTEX_COUNT]);

        return primary_index.erase(key) > 0;
    }

    std::size_t size() const { 
        lock_all_mutexes();
        std::size_t size = container_.size();
        unlock_all_mutexes();
        return size;
    }
    bool empty() const { 
        lock_all_mutexes();
        auto res = container_.empty(); 
        unlock_all_mutexes();
        return res;
    }
    std::size_t capacity() const { return max_size_; }

    void set_capacity(std::size_t new_capacity) {
        max_size_ = new_capacity;
        auto& seq_index = container_.template get<0>();

        lock_all_mutexes();
        while (container_.size() > max_size_) {
            seq_index.pop_back();
        }
        unlock_all_mutexes();
    }

    void clear() { 
        lock_all_mutexes();
        container_.clear(); 
        unlock_all_mutexes();
    }

    template <typename Tag>
    auto end() {
        return container_.template get<Tag>().end();
    }

private:
    using CacheItem = impl::TimestampedValue<Value>;
    using ExtendedIndexSpecifierList = impl::add_seq_index_t<IndexSpecifierList>;
    using BoostContainer = boost::multi_index::multi_index_container<CacheItem, ExtendedIndexSpecifierList, Allocator>;

    void cleanup() {
        lock_all_mutexes();
        auto now = std::chrono::steady_clock::now();
        
        auto& seq_index = container_.template get<0>();
        while(!seq_index.empty()) {
            auto it = seq_index.rbegin(); 
            if (now > it->last_accessed + ttl_) {
                seq_index.pop_back();
            } else {
                break; 
            }
        }
        unlock_all_mutexes();
    }

    void lock_all_mutexes() const {
        for (int i = 0; i < _USERVER_MULTIINDEX_LRU_MUTEX_COUNT; ++i) {
            mutexes_[i].lock();
        }
    }

    void unlock_all_mutexes() const {
        for (int i = 0; i < _USERVER_MULTIINDEX_LRU_MUTEX_COUNT; ++i) {
            mutexes_[i].unlock();
        }
    }

    void start_cleanup() {
        std::lock_guard<std::mutex> lock(start_thread_mutex_);
        if (cleanup_thread_running_.load(std::memory_order_relaxed)) {
            return;
        }

        cleanup_thread_running_.store(true, std::memory_order_release);
        cleanup_thread_ = std::thread([this]() {
            while (cleanup_thread_running_.load(std::memory_order_acquire)) {
                std::this_thread::sleep_for(this->cleanup_interval_);
                this->cleanup();
            }
        });
    }

    void stop_cleanup() {
        cleanup_thread_running_.store(false, std::memory_order_release);
        if (cleanup_thread_.joinable()) {
            cleanup_thread_.join();
        }
    }

    BoostContainer container_;
    std::size_t max_size_;
    std::chrono::milliseconds ttl_;
    std::chrono::milliseconds cleanup_interval_;
    mutable std::mutex start_thread_mutex_;
    std::atomic<bool> cleanup_thread_running_;
    std::thread cleanup_thread_;
    mutable std::array<std::mutex, _USERVER_MULTIINDEX_LRU_MUTEX_COUNT> mutexes_;
};
}  // namespace multi_index_lru

USERVER_NAMESPACE_END
