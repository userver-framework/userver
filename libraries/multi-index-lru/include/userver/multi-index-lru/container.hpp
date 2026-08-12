#pragma once

/// @file userver/multi-index-lru/container.hpp
/// @brief @copybrief multi_index_lru::Container

#include <vector>

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

    /// @brief Constructs element in-place
    /// @returns pair<iterator, bool>
    template <typename... Args>
    auto emplace(Args&&... args) {
        auto& seq_index = get_sequenced();

        std::pair<decltype(seq_index.begin()), bool> result;

        if (free_nodes_.size() > 0) {
            auto node = std::move(free_nodes_.back());
            free_nodes_.pop_back();

            node.value() = Value(std::forward<Args>(args)...);
            auto ret = seq_index.insert(seq_index.begin(), std::move(node));
            result = {ret.position, ret.inserted};
        } else {
            result = seq_index.emplace_front(std::forward<Args>(args)...);
        }
        if (!result.second) {
            seq_index.relocate(seq_index.begin(), result.first);
        } else if (seq_index.size() > max_size_) {
            free_nodes_.emplace_back(seq_index.extract(std::prev(seq_index.end())));
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

    /// @brief Returns range of matching elements, updates all
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

    /// @brief Returns range of matching elements without updates
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

    /// Removes the @b it from container, leaving the node in an internal pool. The key and value are not destroyed
    /// and are reused on next insertion.
    template <typename IteratorType>
    auto erase(IteratorType it) {
        auto it_0 = project_to_sequenced(it);
        auto& seq_index = get_sequenced();
        if (it_0 == seq_index.end()) {
            return it;
        }

        ++it;
        free_nodes_.emplace_back(seq_index.extract(it_0));
        return it;
    }

    /// Removes the @b key from container, leaving the node in an internal pool. The key and value are not destroyed
    /// and are reused on next insertion.
    template <typename Tag, typename Key>
    bool erase(const Key& key) {
        auto it = find_no_update<Tag, Key>(key);
        return erase(it) != end<Tag>();
    }

    std::size_t size() const noexcept { return container_.size(); }

    bool empty() const noexcept { return container_.empty(); }

    std::size_t capacity() const noexcept { return max_size_; }

    void set_capacity(std::size_t new_capacity) {
        max_size_ = new_capacity;
        if (container_.size() <= max_size_) {
            return;
        }

        shrink_to_fit();
        auto& seq_index = get_sequenced();
        while (container_.size() > max_size_) {
            // Not putting the node to `free_nodes_`, because the container is already full
            seq_index.extract(std::prev(seq_index.end()));
        }
    }

    /// Clears the internal nodes pool
    void shrink_to_fit() { free_nodes_.clear(); }

    /// Removes all elements from the container, leaving the node in an internal pool. The keys and values are
    /// not destroyed and are reused on next insertion.
    void clear() {
        auto& seq_index = get_sequenced();
        free_nodes_.reserve(free_nodes_.size() + container_.size());
        while (!container_.empty()) {
            free_nodes_.emplace_back(seq_index.extract(std::prev(seq_index.end())));
        }
    }

    /// @brief Returns end iterator for the specified `Tag` index
    template <typename Tag>
    auto end() const {
        return get_index<Tag>().end();
    }

    auto find_last_accessed_no_update() const { return std::prev(get_sequenced().end()); }

private:
    using ExtendedIndexSpecifierList = impl::add_index_t<boost::multi_index::sequenced<>, IndexSpecifierList>;
    using BoostContainer = boost::multi_index::multi_index_container<Value, ExtendedIndexSpecifierList, Allocator>;

    BoostContainer container_;
    std::size_t max_size_;
    std::vector<typename BoostContainer::node_type> free_nodes_;

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
