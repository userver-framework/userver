#pragma once

#include "boost/multi_index_container.hpp"
#include "boost/multi_index/ordered_index.hpp"
#include "boost/multi_index/sequenced_index.hpp"
#include "boost/multi_index/identity.hpp"
#include "boost/multi_index/member.hpp"
#include "boost/multi_index/tag.hpp"
#include <type_traits>
#include <concepts>
#include <cassert>

template<typename T, typename Tag, typename Key, typename... Args>
concept LRUCacheType = requires(T cache, size_t size, const Key &key, Args&&... args) {
    T{size}; 
    {cache.size()} -> std::same_as<size_t>;
    {cache.empty()} -> std::same_as<bool>;
    {cache.capacity()} -> std::same_as<size_t>;
    {cache.clear()} -> std::same_as<void>;
    {cache.set_capacity(size)} -> std::same_as<void>;

    {cache.template find<Tag>(key)} -> std::input_iterator;
    {cache.template contains<Tag>(key)} -> std::same_as<bool>;
    {cache.template erase<Tag>(key)} -> std::same_as<bool>;
    cache.template get<Tag>();
    std::as_const(cache).template get<Tag>();

    {cache.emplace(std::forward<Args>(args)...)} -> std::same_as<bool>;
};

#define lru_concept_assert_for_one_tag(CahceType, Tag, IndexType, ValueType) \
    static_assert((LRUCacheType<CahceType, Tag, IndexType, const ValueType&>, "LRUCacheType concept")); \
    static_assert((LRUCacheType<CahceType, Tag, IndexType, ValueType&&>, "LRUCacheType concept")); 
