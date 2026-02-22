#pragma once

/// @file userver/multi-index-lru/expirable_container.hpp
/// @brief @copybrief multi_index_lru::ExpirableContainer

#include "impl/mpl_helpers.hpp"
#include "container.hpp"

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace multi_index_lru {

/// @ingroup userver_containers
///
/// @brief MultiIndex LRU expirable container
template <typename Value, typename IndexSpecifierList, typename Allocator>
class ExpirableContainer {
public:
    explicit ExpirableContainer(size_t max_size,
                       std::chrono::milliseconds ttl)
        : container_(max_size), ttl_(ttl)
    {
        UASSERT_MSG(ttl.count() > 0, "ttl must be positive");
    }

    template <typename... Args>
    auto emplace(Args&&... args) {
        auto result = container_.emplace(std::forward<Args>(args)...);

        if (!result.second) {
            result.first->last_accessed = std::chrono::steady_clock::now();
        }

        return result;
    }

    bool insert(const Value& value) { return emplace(value).second; }

    bool insert(Value&& value) { return emplace(std::move(value)).second; }

    template <typename Tag, typename Key>
    auto find(const Key& key) {
        auto now = std::chrono::steady_clock::now();
        auto it = container_.template find<Tag, Key>(key);
        
        if (it != container_.template end<Tag>()) {
            if (now > it->last_accessed + ttl_) {
                container_.template get_index<Tag>().erase(it);
                return end<Tag>();
            } else {
                it->last_accessed = now;
            }
        }
        
        return impl::TimestampedIteratorWrapper{it};
    }

    template <typename Tag, typename Key>
    auto find_no_update(const Key& key) {
        auto it = container_.template find_no_update<Tag, Key>(key);
        return impl::TimestampedIteratorWrapper{it};
    }

    template <typename Tag, typename Key>
    auto equal_range(const Key& key) {
        auto now = std::chrono::steady_clock::now();
        auto& index = container_.template get_index<Tag>();
        auto range = container_.template equal_range<Tag, Key>(key);
        
        auto it = range.first;
        bool changed = false;
        
        while (it != range.second) {
            if (now > it->last_accessed + ttl_) {
                it = index.erase(it);
                changed = true;
            } else {
                it->last_accessed = now;
                ++it;
            }
        }
        if (changed) {
            range = index.equal_range(key);
        }
        return std::pair{impl::TimestampedIteratorWrapper{range.first},
                         impl::TimestampedIteratorWrapper{range.second}};
    }

    template <typename Tag, typename Key>
    auto equal_range_no_update(const Key& key) {
        auto [begin, end] = container_.template equal_range_no_update<Tag, Key>(key);
        return std::pair{impl::TimestampedIteratorWrapper{begin},
                         impl::TimestampedIteratorWrapper{end}};
    }

    template <typename Tag, typename Key>
    bool contains(const Key& key) {
        return this->template find<Tag, Key>(key) != this->template end<Tag>();
    }

    template <typename Tag, typename Key>
    bool erase(const Key& key) {
        return container_.template erase<Tag, Key>(key);
    }

    std::size_t size() const { 
        return container_.size(); 
    }
    bool empty() const { 
        return container_.empty(); 
    }
    std::size_t capacity() const { return container_.capacity(); }

    void set_capacity(std::size_t new_capacity) {
        container_.set_capacity(new_capacity);
    }

    void clear() { 
        container_.clear(); 
    }

    template <typename Tag>
    auto end() {
        return impl::TimestampedIteratorWrapper{container_.template end<Tag>()};
    }

    void cleanup_expired() {
        auto now = std::chrono::steady_clock::now();
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

private:
    using CacheItem = impl::TimestampedValue<Value>;
    using CacheContainer = Container<CacheItem, IndexSpecifierList, Allocator>;

    CacheContainer container_;
    std::chrono::milliseconds ttl_;
};

}  // namespace multi_index_lru

USERVER_NAMESPACE_END