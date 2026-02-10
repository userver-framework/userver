#pragma once

/// @file userver/multi-index-lru/container.hpp
/// @brief @copybrief multi_index_lru::ExpirableContainer

#include <functional>
#include <cassert> 
#include <shared_mutex>
#include <mutex>
#include <optional>

#include "impl/mpl_helpers.hpp"
#include "container.hpp"

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
template <typename Value, typename IndexSpecifierList, typename Allocator>
class ExpirableContainer {
public:
    explicit ExpirableContainer(size_t max_size,
                       std::chrono::milliseconds ttl,
                       std::chrono::milliseconds cleanup_interval = std::chrono::milliseconds(60))
        : container_(max_size), ttl_(ttl), cleanup_interval_(cleanup_interval)
    {
        assert(ttl.count() > 0 && "ttl must be positive");
        assert(cleanup_interval.count() > 0 && "cleanup_interval must be positive");
    }

    ~ExpirableContainer() {
        stop_cleanup();
    }

    template <typename... Args>
    auto emplace(Args&&... args) {
        std::lock_guard<userver::engine::SharedMutex> lock(mutex_);

        auto result = container_.emplace(std::forward<Args>(args)...);

        if (!result.second) {
            result.first->last_accessed = std::chrono::steady_clock::now();
        }

        start_cleanup();

        return result;
    }

    bool insert(const Value& value) { return emplace(value).second; }

    bool insert(Value&& value) { return emplace(std::move(value)).second; }

    template <typename Tag, typename Key>
    std::optional<Value> get(const Key& key) {
        std::lock_guard<userver::engine::SharedMutex> lock(mutex_);
        auto it = find<Tag, Key>(lock, key);
        if (it == end<Tag>()) {
            return std::nullopt;
        }
        return *it;
    }

    template <typename Tag, typename Key>
    bool contains(const Key& key) {
        std::lock_guard<userver::engine::SharedMutex> lock(mutex_);
        return this->template find<Tag, Key>(lock, key) != container_.template end<Tag>();
    }

    template <typename Tag, typename Key>
    bool erase(const Key& key) {
        std::lock_guard<userver::engine::SharedMutex> lock(mutex_);
        return container_.template erase<Tag, Key>(key);
    }

    std::size_t size() const { 
        std::shared_lock<userver::engine::SharedMutex> lock(mutex_);
        return container_.size(); 
    }
    bool empty() const { 
        std::shared_lock<userver::engine::SharedMutex> lock(mutex_);
        return container_.empty(); 
    }
    std::size_t capacity() const { return container_.capacity(); }

    void set_capacity(std::size_t new_capacity) {
        std::lock_guard<userver::engine::SharedMutex> lock(mutex_);
        container_.set_capacity(new_capacity);
    }

    void clear() { 
        std::lock_guard<userver::engine::SharedMutex> lock(mutex_);
        container_.clear(); 
    }

    template <typename Tag>
    auto end() {
        return container_.template end<Tag>();
    }

private:
    using CacheItem = impl::TimestampedValue<Value>;
    using ExtendedIndexSpecifierList = impl::add_index_t<
                                    boost::multi_index::sequenced<>,
                                    IndexSpecifierList>;
    using CacheContainer = Container<CacheItem, IndexSpecifierList, Allocator>;

    template <typename Tag, typename Key>
    auto find(std::lock_guard<userver::engine::SharedMutex>&, const Key& key) {
        auto it = container_.template find<Tag, Key>(key);
        
        if (it != container_.template end<Tag>()) {
            if (std::chrono::steady_clock::now() > it->last_accessed + ttl_) {
                container_.template get_index<Tag>().erase(it);
                return impl::TimestampedIteratorWrapper{container_.template end<Tag>()};
            }

            it->last_accessed = std::chrono::steady_clock::now();
        }

        return impl::TimestampedIteratorWrapper{it};
    }

    void cleanup() {
        auto now = std::chrono::steady_clock::now();
        std::lock_guard<userver::engine::SharedMutex> lock(mutex_);
        
        auto& seq_index = container_.get_sequensed();
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

    CacheContainer container_;
    std::chrono::milliseconds ttl_;
    std::chrono::milliseconds cleanup_interval_;
    mutable userver::engine::SharedMutex mutex_;
    userver::engine::Task cleanup_task_; 
};


}  // namespace multi_index_lru

USERVER_NAMESPACE_END