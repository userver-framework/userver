#pragma once

/// @file userver/multi-index-lru/container.hpp
/// @brief @copybrief multi_index_lru::Container

#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index_container.hpp> 
#include <boost/multi_index/ordered_index.hpp>

#include <boost/mpl/list.hpp>
#include <boost/mpl/joint_view.hpp>
#include <boost/mpl/aux_/na.hpp>

#include <utility>
#include <cstddef>
#include <tuple>

USERVER_NAMESPACE_BEGIN

namespace multi_index_lru {

namespace impl {
template<typename T>
constexpr bool is_mpl_na = std::is_same_v<T, boost::mpl::na>;

template<typename First, typename... Rest>
struct filter_na {
    using rest_filtered = typename filter_na<Rest...>::type;

    using type = std::conditional_t<
        is_mpl_na<First>,
        rest_filtered,
        decltype(std::tuple_cat(std::tuple<First>(), rest_filtered()))
    >;
};

template<typename First>
struct filter_na<First> {
    using type = std::conditional_t<
        is_mpl_na<First>,
        std::tuple<>,
        std::tuple<First>
    >;
};

template<typename IndexList> struct add_seq_index {};

template<typename... Indices>
struct add_seq_index<boost::multi_index::indexed_by<Indices...>> {
private:
    using filtered = typename filter_na<Indices...>::type;

    template<typename Tuple>
    struct wrap_tuple;

    template<typename... Ts>
    struct wrap_tuple<std::tuple<Ts...>> {
        using type = boost::multi_index::indexed_by<
            boost::multi_index::sequenced<>,
            Ts...
        >;
    };

public:
    using type = typename wrap_tuple<filtered>::type;
};

template<typename IndexList>
using add_seq_index_t = typename add_seq_index<IndexList>::type;
} // namespace impl

/// @ingroup userver_containers
///
/// @brief MultiIndex LRU container
template <typename Value, typename IndexSpecifierList, typename Allocator = std::allocator<Value> >
class Container {
public:
    explicit Container(size_t max_size)
        : max_size_(max_size)
    {}

    template<typename... Args>
    bool emplace(Args&&... args) {
        auto& seq_index = container_.template get<0>();
        auto result = seq_index.emplace_front(std::forward<Args>(args)...);

        if (!result.second) {
            seq_index.relocate(seq_index.begin(), result.first);
        } else if (seq_index.size() > max_size_) {
            seq_index.pop_back();
        }
        return result.second;
    }

    bool insert(const Value& value) { return emplace(value); }

    bool insert(Value&& value) { return emplace(std::move(value)); }

    template <typename Tag, typename Key>
    auto find(const Key& key) {
        auto& primary_index = container_.template get<Tag>();
        auto it = primary_index.find(key);

        if (it != primary_index.end()) {
            auto& seq_index = container_.template get<0>();
            auto seq_it = container_.template project<0>(it);
            seq_index.relocate(seq_index.begin(), seq_it);
        }

        return it;
    }

    template <typename Tag, typename Key>
    bool contains(const Key& key) {
        return this->template find<Tag, Key>(key) != container_.template get<Tag>().end();
    }

    template <typename Tag, typename Key>
    bool erase(const Key& key) {
        return container_.template get<Tag>().erase(key) > 0;
    }

    std::size_t size() const { return container_.size(); }
    bool empty() const { return container_.empty(); }
    std::size_t capacity() const { return max_size_; }

    void set_capacity(std::size_t new_capacity) {
        max_size_ = new_capacity;
        auto& seq_index = container_.template get<0>();
        while (container_.size() > max_size_) {
            seq_index.pop_back();
        }
    }

    void clear() { container_.clear(); }

    template <typename Tag>
    template<typename Tag>
    auto end() {
        return container_.template get<Tag>().end();
    }

private:
    using ExtendedIndexSpecifierList = impl::add_seq_index_t<IndexSpecifierList>;

    using BoostContainer = boost::multi_index::multi_index_container<
        Value,
        ExtendedIndexSpecifierList,
        Allocator
    >;

    BoostContainer container_;
    std::size_t max_size_;
};
} // namespace multi_index_lru

USERVER_NAMESPACE_END