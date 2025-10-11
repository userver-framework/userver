#pragma once

#include <boost/intrusive/link_mode.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/list_hook.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <list>
#include <functional>
#include <unordered_set>
#include <iostream>

namespace lru_boost_list {
using namespace boost::multi_index;

template<typename Value>
struct ValueWithHook : public boost::intrusive::list_base_hook
                                                        <boost::intrusive::link_mode
                                                            <boost::intrusive::safe_link>>
{
    static size_t id;
    Value value;
    size_t internal_id;

    ValueWithHook() : internal_id(++id) {}
    
    explicit ValueWithHook(const Value& val) 
        : value(val), internal_id(++id) {
            #ifdef LRU_CONTAINER_DEBUG__
            std::cout << "created id " << internal_id << std::endl;
            #endif
        }
        
    explicit ValueWithHook(Value&& val) 
        : value(std::move(val)), internal_id(++id) {
            #ifdef LRU_CONTAINER_DEBUG__
            std::cout << "created id " << internal_id << std::endl;
            #endif
        }
    
    operator Value&() { return value; }
    operator const Value&() const { return value; }
    
    Value* operator->() { return &value; }
    const Value* operator->() const { return &value; }
    
    Value& get() { return value; }
    const Value& get() const { return value; }
};

struct internal_id_tag {};

template<
    typename Value,
    typename IndexSpecifierList,
    typename Allocator = std::allocator<ValueWithHook<Value>>
>
class LRUCacheContainer_BoostList {
private:
    using CacheItem = ValueWithHook<Value>;
    using List = boost::intrusive::list<ValueWithHook<Value>>;

    using ExtendedIndexSpecifierList = typename boost::mpl::push_back<
        IndexSpecifierList,
        hashed_unique<
            tag<internal_id_tag>,
            member<CacheItem, size_t, &CacheItem::internal_id>
        >
    >::type;

    using Container = multi_index_container<
        CacheItem,
        ExtendedIndexSpecifierList,
        Allocator
    >;

    Container container;
    size_t max_size;

    List usage_list;
    
public:
    using value_type = Value;
    using cache_item_type = CacheItem;
    
    LRUCacheContainer_BoostList(size_t max_size) : max_size(max_size) {}
    
    template<typename... Args>
    bool emplace(Args&&... args) {
        if (container.size() >= max_size) {
            evict_lru();
        }
        
        auto result = container.emplace(std::forward<Args>(args)...);
        auto &value = const_cast<ValueWithHook<Value>&>(*result.first);
        if (result.second) {
            usage_list.push_back(value);
        } else {
            touch(value);
        }
        return result.second;
    }
    
    bool insert(const Value& value) {
        return emplace(value);
    }
    
    bool insert(Value&& value) {
        return emplace(std::move(value));
    }
    
    template<typename Tag, typename Key>
    typename Container::template index<Tag>::type::iterator find(const Key& key) {
        auto& primary_index = container.template get<Tag>();
        auto it = primary_index.find(key);
        
        if (it != primary_index.end()) {
            auto &value = const_cast<ValueWithHook<Value>&>(*it);
            touch(value);
        }
        
        return it;
    }
    
    template<typename Tag, typename Key>
    bool contains(const Key& key) {
        return this->template find<Tag, Key>(key) != container.template get<Tag>().end();
    }
    
    template<typename Tag, typename Key>
    bool erase(const Key& key) {
        auto& primary_index = container.template get<Tag>();
        auto it = primary_index.find(key);
        if (it != primary_index.end()) {
            usage_list.erase(usage_list.iterator_to(*it));
        }
        return container.template get<Tag>().erase(key) > 0;
    }
    
    template<typename Tag>
    auto& get() {
        return container.template get<Tag>();
    }
    
    template<typename Tag>
    const auto& get() const {
        return container.template get<Tag>();
    }
    
    size_t size() const { return container.size(); }
    bool empty() const { return container.empty(); }
    size_t capacity() const { return max_size; }
    
    void set_capacity(size_t new_capacity) {
        max_size = new_capacity;
        while (container.size() > max_size) {
            evict_lru();
        }
    }
    
    void clear() {
        container.clear();
    }
    
private:
    void evict_lru() {
        if (!usage_list.empty()) {
            size_t id_to_erase = usage_list.begin()->internal_id;
            usage_list.erase(usage_list.begin());
            container.template get<internal_id_tag>().erase(id_to_erase);
        }
    }

    void touch(CacheItem &item) {
        auto it = usage_list.iterator_to(item);
        if (it != usage_list.end()) {
            usage_list.splice(usage_list.end(), usage_list, it);
        }
    }
};

template<typename T>
size_t ValueWithHook<T>::id = 0;
}