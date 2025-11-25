#pragma once

/// @file userver/multi-index-lru/container.hpp
/// @brief @copybrief multi_index_lru::Container

#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index_container.hpp> 
#include <boost/multi_index/ordered_index.hpp>

#include <boost/mpl/list.hpp>
#include <boost/mpl/joint_view.hpp>

#include <utility>
#include <cstddef>

USERVER_NAMESPACE_BEGIN

namespace multi_index_lru {


/// @ingroup userver_containers
///
/// @brief MultiIndex LRU container
template<
    typename Value,
    typename IndexSpecifierList,
    typename Allocator = std::allocator<Value>
>
class Container {
public:
    explicit Container(size_t max_size) : max_size(max_size) {}
    
    template<typename... Args>
    bool emplace(Args&&... args) {
        auto result = container.emplace_front(std::forward<Args>(args)...);

        if (result.second == false) {
            container.relocate(container.begin(),result.first);
        } else if (container.size() >= max_size) {
            container.pop_back();
        }
        return result.second;
    }
    
    bool insert(const Value& value) { return emplace(value); }

    bool insert(Value&& value) { return emplace(std::move(value)); }
    
    template<typename Tag, typename Key>
    auto find(const Key& key) {
        auto& primary_index = container.template get<Tag>();
        auto it = primary_index.find(key);
        
        if (it != primary_index.end()) {
            container.relocate(container.begin(),it);
        }
        
        return it;
    }
    
    template<typename Tag, typename Key>
    bool contains(const Key& key) {
        return this->template find<Tag, Key>(key) != container.template get<Tag>().end();
    }
    
    template<typename Tag, typename Key>
    bool erase(const Key& key) {
        return container.template get<Tag>().erase(key) > 0;
    }
    
    size_t size() const { return container.size(); }
    bool empty() const { return container.empty(); }
    size_t capacity() const { return max_size; }
    
    void set_capacity(size_t new_capacity) {
        max_size = new_capacity;
        while (container.size() > max_size) {
            container.pop_back();
        }
    }
    
    void clear() {
        container.clear();
    }

private:
    using AdditionalIndices = boost::mpl::list<
        boost::multi_index::sequenced<>
    >;

    using ExtendedIndexSpecifierList =
        boost::mpl::joint_view<IndexSpecifierList, AdditionalIndices>;

    using BoostContainer = boost::multi_index::multi_index_container<
        Value,
        ExtendedIndexSpecifierList,
        Allocator
    >;

    BoostContainer container;
    size_t max_size;
};
} // namespace multi_index_lru

USERVER_NAMESPACE_END