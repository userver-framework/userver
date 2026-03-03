#pragma once

/// @file userver/multi-index-lru/container.hpp
/// @brief @copybrief multi_index_lru::Container

#include "impl/mpl_helpers.hpp"

USERVER_NAMESPACE_BEGIN

namespace multi_index_lru {

template <typename Value, typename IndexSpecifierList, typename Allocator = std::allocator<Value>>
class ExpirableContainer;

/// @ingroup userver_containers
///
/// @brief MultiIndex LRU container
template <typename Value, typename IndexSpecifierList, typename Allocator = std::allocator<Value>>
class Container {
public:
    explicit Container(size_t max_size)
        : max_size_(max_size)
    {}

    template <typename... Args>
    auto emplace(Args&&... args) {
        auto& seq_index = get_sequenced();
        auto result = seq_index.emplace_front(std::forward<Args>(args)...);

        if (!result.second) {
            seq_index.relocate(seq_index.begin(), result.first);
        } else if (seq_index.size() > max_size_) {
            seq_index.pop_back();
        }
        return result;
    }

    bool insert(const Value& value) { return emplace(value).second; }

    bool insert(Value&& value) { return emplace(std::move(value)).second; }

    template <typename Tag, typename Key>
    auto find(const Key& key) {
        auto& primary_index = get_index<Tag>();
        auto it = primary_index.find(key);

        if (it != primary_index.end()) {
            auto& seq_index = get_sequenced();
            auto seq_it = container_.template project<0>(it);
            seq_index.relocate(seq_index.begin(), seq_it);
        }

        return it;
    }

    template <typename Tag, typename Key>
    auto find_no_update(const Key& key) const {
        return get_index<Tag>().find(key);
    }

    template <typename Tag, typename Key>
    auto equal_range(const Key& key) {
        auto& primary_index = get_index<Tag>();

        auto [begin, end] = primary_index.equal_range(key);
        auto it = begin;

        auto& seq_index = get_sequenced();
        while (it != end) {
            seq_index.relocate(seq_index.begin(), project_to_sequenced(it));
            ++it;
        }

        return std::pair{begin, end};
    }

    template <typename Tag, typename Key>
    auto equal_range_no_update(const Key& key) const {
        return get_index<Tag>().equal_range(key);
    }

    template <typename Tag, typename Key>
    bool contains(const Key& key) {
        return this->template find<Tag, Key>(key) != end<Tag>();
    }

    template <typename Tag, typename Key>
    bool contains_no_update(const Key& key) const {
        return this->template find_no_update<Tag, Key>(key) != end<Tag>();
    }

    template <typename Tag, typename Key>
    bool erase(const Key& key) {
        return get_index<Tag>().erase(key) > 0;
    }

    std::size_t size() const noexcept { return container_.size(); }
    bool empty() const noexcept { return container_.empty(); }
    std::size_t capacity() const noexcept { return max_size_; }

    void set_capacity(std::size_t new_capacity) {
        max_size_ = new_capacity;
        auto& seq_index = get_sequenced();
        while (container_.size() > max_size_) {
            seq_index.pop_back();
        }
    }

    void clear() { container_.clear(); }

    template <typename Tag>
    auto end() const {
        return get_index<Tag>().end();
    }

private:
    template <typename V, typename I, typename A>
    friend class ExpirableContainer;

    using ExtendedIndexSpecifierList = impl::add_index_t<boost::multi_index::sequenced<>, IndexSpecifierList>;
    using BoostContainer = boost::multi_index::multi_index_container<Value, ExtendedIndexSpecifierList, Allocator>;

    BoostContainer container_;
    std::size_t max_size_;

    auto& get_sequenced() noexcept { return container_.template get<0>(); }

    const auto& get_sequenced() const noexcept { return container_.template get<0>(); }

    template <typename Tag>
    auto& get_index() {
        return container_.template get<Tag>();
    }

    template <typename Tag>
    const auto& get_index() const noexcept {
        return container_.template get<Tag>();
    }

    template <typename IterT>
    auto project_to_sequenced(IterT it) {
        return container_.template project<0>(it);
    }
};

}  // namespace multi_index_lru

USERVER_NAMESPACE_END
