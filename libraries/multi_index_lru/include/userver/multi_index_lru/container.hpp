#pragma once

#include <boost/intrusive/link_mode.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/list_hook.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index_container.hpp> 

#include <utility>
#include <cstddef>

USERVER_NAMESPACE_BEGIN

namespace multi_index_lru {

namespace impl {
template<typename Value>
struct ValueWithHook
{
    Value value;
    mutable boost::intrusive::list_member_hook<> list_hook;
    using boost_list = boost::intrusive::list<
                            ValueWithHook,
                            boost::intrusive::member_hook<
                                ValueWithHook,
                                boost::intrusive::list_member_hook<>, 
                                &ValueWithHook::list_hook
                            >
                        >;

    const ValueWithHook *GetPointerToSelf() const { return this; };

    explicit ValueWithHook(const Value& val) : value(val) {}
        
    explicit ValueWithHook(Value&& val) : value(std::move(val)) {}
    
    ValueWithHook() = delete;
    ValueWithHook(const ValueWithHook&) = delete;
    ValueWithHook(ValueWithHook &&) = delete;

    ValueWithHook &operator=(const ValueWithHook&) = delete;
    ValueWithHook &operator=(ValueWithHook&&) = delete;
    
    operator Value&() { return value; }
    operator const Value&() const { return value; }
    
    Value* operator->() { return &value; }
    const Value* operator->() const { return &value; }

    Value& get() { return value; }
    const Value& get() const { return value; }

    void PushBackToList(boost_list &lst) const {
        lst.push_back(const_cast<ValueWithHook&>(*this));
    }

    void SpliceInList(boost_list &lst) const {
        lst.splice(lst.end(), lst, lst.iterator_to(const_cast<ValueWithHook&>(*this)));
    }
};

struct internalPtrTag {};
} // namespace impl

template<
    typename Value,
    typename IndexSpecifierList,
    typename Allocator = std::allocator<impl::ValueWithHook<Value>>
>
class LRUCacheContainer {
public:
    explicit LRUCacheContainer(size_t max_size) : max_size(max_size) {}
    
    template<typename... Args>
    bool emplace(Args&&... args) {
        if (container.size() >= max_size) {
            evict_lru();
        }
        
        auto result = container.emplace(std::forward<Args>(args)...);

        auto &value = *result.first;
        if (result.second) {
            value.PushBackToList(usage_list);
        } else {
            value.SpliceInList(usage_list);
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
    auto find(const Key& key) {
        auto& primary_index = container.template get<Tag>();
        auto it = primary_index.find(key);
        
        if (it != primary_index.end()) {
            it->SpliceInList(usage_list);
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
    using CacheItem = impl::ValueWithHook<Value>;
    using List =  boost::intrusive::list<
                        CacheItem,
                        boost::intrusive::member_hook<
                            CacheItem,
                            boost::intrusive::list_member_hook<>, 
                            &CacheItem::list_hook
                        >
                    >;

    using ExtendedIndexSpecifierList = typename boost::mpl::push_back<
        IndexSpecifierList,
        boost::multi_index::hashed_unique<
            boost::multi_index::tag<impl::internalPtrTag>,
            boost::multi_index::const_mem_fun<CacheItem, const CacheItem*, &CacheItem::GetPointerToSelf>
        >
    >::type;

    using Container = boost::multi_index::multi_index_container<
        CacheItem,
        ExtendedIndexSpecifierList,
        Allocator
    >;

    void evict_lru() {
        if (!usage_list.empty()) {
            CacheItem *ptr_to_erase = &*usage_list.begin();
            usage_list.erase(usage_list.begin());
            container.template get<impl::internalPtrTag>().erase(ptr_to_erase);
        }
    }

    Container container;
    size_t max_size;
    List usage_list;
};
} // namespace multi_index_lru

USERVER_NAMESPACE_END