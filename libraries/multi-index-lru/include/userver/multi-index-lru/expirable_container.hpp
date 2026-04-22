#pragma once

/// @file userver/multi-index-lru/expirable_container.hpp
/// @brief @copybrief multi_index_lru::ExpirableContainer

#include "container.hpp"
#include "impl/mpl_helpers.hpp"

#include <userver/utils/assert.hpp>
#include <userver/utils/datetime.hpp>

USERVER_NAMESPACE_BEGIN

namespace multi_index_lru {

/// @ingroup userver_containers
///
/// @brief MultiIndex LRU expirable container
template <typename Value, typename IndexSpecifierList, typename Allocator>
class ExpirableContainer {
public:
    explicit ExpirableContainer(size_t max_size, std::chrono::milliseconds ttl)
        : container_(max_size),
          ttl_(ttl)
    {
        UINVARIANT(ttl.count() > 0, "ttl must be positive");
        UINVARIANT(max_size > 0, "capacity must be positive");
    }

    template <typename... Args>
    auto emplace(Args&&... args) {
        const auto now = utils::datetime::SteadyClock::now();
        auto result = container_.emplace(now, std::forward<Args>(args)...);

        if (!result.second) {
            result.first->last_accessed = now;
        }

        return result;
    }

    bool insert(const Value& value) { return emplace(value).second; }

    bool insert(Value&& value) { return emplace(std::move(value)).second; }

    template <typename Tag, typename Key>
    auto find(const Key& key) {
        const auto now = utils::datetime::SteadyClock::now();
        auto it = container_.template find<Tag, Key>(key);

        if (it != container_.template end<Tag>()) {
            if (now > it->last_accessed + ttl_) {
                container_.erase(it);
                return end<Tag>();
            } else {
                it->last_accessed = now;
            }
        }

        return impl::IteratorToValue{it};
    }

    template <typename Tag, typename Key>
    auto find_no_update(const Key& key) const {
        auto it = container_.template find_no_update<Tag, Key>(key);
        return impl::IteratorToValue{it};
    }

    template <typename Tag, typename Key>
    auto equal_range(const Key& key) {
        const auto now = utils::datetime::SteadyClock::now();
        auto range = container_.template equal_range<Tag, Key>(key);

        auto it = range.first;
        bool changed = false;

        while (it != range.second) {
            if (now > it->last_accessed + ttl_) {
                it = container_.erase(it);
                changed = true;
            } else {
                it->last_accessed = now;
                ++it;
            }
        }
        if (changed) {
            range = container_.template equal_range_no_update<Tag, Key>(key);
        }
        return std::pair{impl::IteratorToValue{range.first}, impl::IteratorToValue{range.second}};
    }

    template <typename Tag, typename Key>
    auto equal_range_no_update(const Key& key) const {
        auto [begin, end] = container_.template equal_range_no_update<Tag, Key>(key);
        return std::pair{impl::IteratorToValue{begin}, impl::IteratorToValue{end}};
    }

    template <typename Tag, typename Key>
    bool contains(const Key& key) {
        return this->template find<Tag, Key>(key) != this->template end<Tag>();
    }

    template <typename Tag, typename Key>
    bool contains_no_update(const Key& key) const {
        return this->template find_no_update<Tag, Key>(key) != this->template end<Tag>();
    }

    /// Removes the @b it from container, leaving the node in an internal pool. The key and value are not destroyed
    /// and are reused on next insertion.
    template <typename IteratorType>
    bool erase(IteratorType it) {
        return container_.erase(it);
    }

    /// Removes the @b key from container, leaving the node in an internal pool. The key and value are not destroyed
    /// and are reused on next insertion.
    template <typename Tag, typename Key>
    bool erase(const Key& key) {
        return container_.template erase<Tag, Key>(key);
    }

    std::size_t size() const noexcept { return container_.size(); }

    bool empty() const noexcept { return container_.empty(); }

    std::size_t capacity() const noexcept { return container_.capacity(); }

    void set_capacity(std::size_t new_capacity) { container_.set_capacity(new_capacity); }

    /// Clears the internal nodes pool
    void shrink_to_fit() { container_.shrink_to_fit(); }

    /// Removes all elements from the container, leaving the node in an internal pool. The keys and values are
    /// not destroyed and are reused on next insertion.
    void clear() { container_.clear(); }

    template <typename Tag>
    auto end() {
        return impl::IteratorToValue{container_.template end<Tag>()};
    }

    /// @brief Removes all expired items from container
    void cleanup_expired() {
        const auto now = utils::datetime::SteadyClock::now();

        while (!container_.empty()) {
            const auto it = container_.find_last_accessed_no_update();
            if (now > it->last_accessed + ttl_) {
                container_.erase(it);
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
