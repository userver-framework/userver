#pragma once

#include <chrono>

USERVER_NAMESPACE_BEGIN

namespace lru_time_index {
using namespace boost::multi_index;

struct lru_time_tag {};

template<typename Value>
struct TimestampedValue {
    Value value;
    std::chrono::steady_clock::time_point last_accessed;
    
    TimestampedValue() = default;
    
    explicit TimestampedValue(const Value& val) 
        : value(val), last_accessed(std::chrono::steady_clock::now()) {}
        
    explicit TimestampedValue(Value&& val) 
        : value(std::move(val)), last_accessed(std::chrono::steady_clock::now()) {}
    
    operator Value&() { return value; }
    operator const Value&() const { return value; }
    
    Value* operator->() { return &value; }
    const Value* operator->() const { return &value; }
    
    Value& get() { return value; }
    const Value& get() const { return value; }
};

template<
    typename Value,
    typename IndexSpecifierList,
    typename Allocator = std::allocator<TimestampedValue<Value>>
>
class LRUCacheContainer_TimeIndex {
private:
    using CacheItem = TimestampedValue<Value>;
    
    using ExtendedIndexSpecifierList = typename boost::mpl::push_back<
        IndexSpecifierList,
        ordered_non_unique<
            tag<lru_time_tag>,
            member<CacheItem, std::chrono::steady_clock::time_point, &CacheItem::last_accessed>
        >
    >::type;

    using Container = multi_index_container<
        CacheItem,
        ExtendedIndexSpecifierList,
        Allocator
    >;

    Container container;
    size_t max_size;
    
public:
    using value_type = Value;
    using cache_item_type = CacheItem;
    
    LRUCacheContainer_TimeIndex(size_t max_size) : max_size(max_size) {}
    
    template<typename... Args>
    bool emplace(Args&&... args) {
        if (container.size() >= max_size) {
            evict_lru();
        }
        
        auto result = container.emplace(std::forward<Args>(args)...);
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
            primary_index.modify(it, [](CacheItem& item) {
                item.last_accessed = std::chrono::steady_clock::now();
            });
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
        auto& time_based_index = container.template get<lru_time_tag>();
        
        if (!time_based_index.empty()) {
            time_based_index.erase(time_based_index.begin());
        }
    }
};
}

USERVER_NAMESPACE_END