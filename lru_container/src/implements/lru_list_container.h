#pragma once

#include <boost/intrusive/link_mode.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <list>
#include <functional>
#include <unordered_set>

namespace lru_list {
using namespace boost::multi_index;

template<typename Value>
struct ValueWithIdentificator {
    static size_t id;
    Value value;
    size_t internal_id;
    
    // тестовая реализация, я понимаю, что вариант с таким стаким счетчиком 
    // работает только до переполнения size_t.
    // пока думаю, как сделать лучше, например можно 
    // обязать Value иметь поле ::key хэшируемого типа 
    ValueWithIdentificator() : internal_id(++id) {};
    
    explicit ValueWithIdentificator(const Value& val) 
        : value(val), internal_id(++id) {}
        
    explicit ValueWithIdentificator(Value&& val) 
        : value(std::move(val)), internal_id(++id) {}
    
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
    typename Allocator = std::allocator<ValueWithIdentificator<Value>>
>
class LRUCacheContainer_List {
private:
    using CacheItem = ValueWithIdentificator<Value>;

    // реализация для тестирования подхода с list, я понимаю, что у 
    // std::list линейный поиск по элементу 
    // (т.е. сильно хуже ассимптотика, что видно и по бенчмаркам)
    using List = std::list<size_t>;
    
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

    List usage_id_list;
    
public:
    using value_type = Value;
    using cache_item_type = CacheItem;
    
    LRUCacheContainer_List(size_t max_size) : max_size(max_size) {}
    
    template<typename... Args>
    bool emplace(Args&&... args) {
        if (container.size() >= max_size) {
            evict_lru();
        }
        
        auto result = container.emplace(std::forward<Args>(args)...);
        if (result.second) {
            usage_id_list.insert(usage_id_list.end(), result.first->internal_id);
        } else {
            touch(result.first->internal_id);
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
            touch(it->internal_id);
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
            auto list_it = std::find(usage_id_list.begin(), usage_id_list.end(), it->internal_id);
            if (list_it != usage_id_list.end()) {
                usage_id_list.erase(list_it);
            }
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
        if (!usage_id_list.empty()) {
            size_t id_to_erase = *usage_id_list.begin();
            container.template get<internal_id_tag>().erase(id_to_erase);
            usage_id_list.erase(usage_id_list.begin());
        }
    }

    void touch(size_t key) {
        auto it = std::find(usage_id_list.begin(), usage_id_list.end(), key);
        if (it != usage_id_list.end()) {
            usage_id_list.splice(usage_id_list.end(), usage_id_list, it);
        }
    }
};

template<typename T>
size_t ValueWithIdentificator<T>::id = 0;

}