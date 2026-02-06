#pragma once

/// @file userver/multi-index-lru/container.hpp
/// @brief @copybrief multi_index_lru::ExpirableContainer

#include <functional>
#include <cassert> 
#include <shared_mutex>
#include <mutex>

#include "impl/mpl_helpers.hpp"

#include <userver/utils/async.hpp>
#include <userver/utils/rand.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/shared_mutex.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>

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
        : max_size_(max_size), ttl_(ttl), cleanup_interval_(cleanup_interval)
    {
        assert(ttl.count() > 0 && "ttl must be positive");
        assert(cleanup_interval.count() > 0 && "cleanup_interval must be positive");
    }

    ~ExpirableContainer() {
        stop_cleanup();
    }

    template <typename... Args>
    bool emplace(Args&&... args) {
        std::lock_guard<userver::engine::SharedMutex> read_lock(read_mutex_);
        std::lock_guard<userver::engine::Mutex> write_lock(write_mutex_);

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

        start_cleanup();

        return result.second;
    }

    template <typename Tag, typename Key>
    auto find(const Key& key) {
        std::shared_lock<userver::engine::SharedMutex> read_lock(read_mutex_);
        auto& primary_index = container_.template get<Tag>();
        auto it = primary_index.find(key);

        if (it != primary_index.end()) {
            if (std::chrono::steady_clock::now() > it->last_accessed + ttl_) {
                std::lock_guard<userver::engine::Mutex> write_lock(write_mutex_);
                primary_index.erase(it);
                return impl::TimestampedIteratorWrapper{primary_index.end()};
            }

            auto& seq_index = container_.template get<0>();
            auto seq_it = container_.template project<0>(it);
            {
                std::lock_guard<userver::engine::Mutex> write_lock(write_mutex_);
                seq_index.relocate(seq_index.begin(), seq_it);

                primary_index.modify(it, [](CacheItem& item) {
                    item.last_accessed = std::chrono::steady_clock::now();
                });
            }
        }

        return impl::TimestampedIteratorWrapper{it};
    }

    bool insert(const Value& value) { return emplace(value); }

    bool insert(Value&& value) { return emplace(std::move(value)); }

    template <typename Tag, typename Key>
    bool contains(const Key& key) {
        return this->template find<Tag, Key>(key) != container_.template get<Tag>().end();
    }

    template <typename Tag, typename Key>
    bool erase(const Key& key) {
        std::lock_guard<userver::engine::SharedMutex> read_lock(read_mutex_);
        std::lock_guard<userver::engine::Mutex> write_lock(write_mutex_);
        return container_.template get<Tag>().erase(key) > 0;
    }

    std::size_t size() const { 
        std::shared_lock<userver::engine::SharedMutex> read_lock(read_mutex_);
        return container_.size(); 
    }
    bool empty() const { 
        std::shared_lock<userver::engine::SharedMutex> read_lock(read_mutex_);
        return container_.empty(); 
    }
    std::size_t capacity() const { return max_size_; }

    void set_capacity(std::size_t new_capacity) {
        max_size_ = new_capacity;
        auto& seq_index = container_.template get<0>();

        std::lock_guard<userver::engine::SharedMutex> read_lock(read_mutex_);
        std::lock_guard<userver::engine::Mutex> write_lock(write_mutex_);
        while (container_.size() > max_size_) {
            seq_index.pop_back();
        }
    }

    void clear() { 
        std::lock_guard<userver::engine::SharedMutex> read_lock(read_mutex_);
        std::lock_guard<userver::engine::Mutex> write_lock(write_mutex_);
        container_.clear(); 
    }

    template <typename Tag>
    auto end() {
        return container_.template get<Tag>().end();
    }

private:
    using CacheItem = impl::TimestampedValue<Value>;
    using ExtendedIndexSpecifierList = impl::add_index_t<
                                    boost::multi_index::sequenced<>,
                                    IndexSpecifierList>;
    using BoostContainer = boost::multi_index::multi_index_container<CacheItem, ExtendedIndexSpecifierList, Allocator>;

    void cleanup() {
        std::lock_guard<userver::engine::SharedMutex> read_lock(read_mutex_);
        std::lock_guard<userver::engine::Mutex> write_lock(write_mutex_);
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
    }
    
    void start_cleanup() {
        if (cleanup_task_.IsValid() && !cleanup_task_.IsFinished()) {
            return;
        }

        cleanup_task_ = userver::utils::Async("lru_cleanup", [this] {
            while (!userver::engine::current_task::ShouldCancel()) {
                userver::engine::SleepFor(cleanup_interval_);
                this->cleanup();
            }
        });
    }

    void stop_cleanup() {
        if (cleanup_task_.IsValid()) {
            cleanup_task_.RequestCancel();
            cleanup_task_.Wait(); 
        }
    }

    BoostContainer container_;
    std::size_t max_size_;
    std::chrono::milliseconds ttl_;
    std::chrono::milliseconds cleanup_interval_;
    mutable userver::engine::SharedMutex read_mutex_;
    mutable userver::engine::Mutex write_mutex_;
    userver::engine::Task cleanup_task_; 
};


}  // namespace multi_index_lru

USERVER_NAMESPACE_END