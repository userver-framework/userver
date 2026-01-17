#pragma once

/// @file userver/multi-index-lru/container.hpp
/// @brief @copybrief multi_index_lru::Container

#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>

#include <cstddef>
#include <tuple>
#include <utility>

USERVER_NAMESPACE_BEGIN

namespace multi_index_lru {

namespace impl {
template <typename T, typename = std::void_t<>>
inline constexpr bool is_mpl_na = false;

template <typename T>
inline constexpr bool is_mpl_na<T, std::void_t<decltype(std::declval<T>().~na())>> = true;

template <typename... Indices>
struct lazy_add_seq {
    using type = boost::multi_index::indexed_by<boost::multi_index::sequenced<>, Indices...>;
};

template <typename... Indices>
struct lazy_add_seq_no_last {
private:
    template <std::size_t... I>
    static auto makeWithoutLast(std::index_sequence<I...>) {
        using Tuple = std::tuple<Indices...>;
        return boost::multi_index::indexed_by<boost::multi_index::sequenced<>, std::tuple_element_t<I, Tuple>...>{};
    }

public:
    using type = decltype(makeWithoutLast(std::make_index_sequence<sizeof...(Indices) - 1>{}));
};

template <typename IndexList>
struct add_seq_index {};

template <typename... Indices>
struct add_seq_index<boost::multi_index::indexed_by<Indices...>> {
    using LastType = decltype((Indices{}, ...));

    using type = typename std::conditional_t<
        is_mpl_na<LastType>,
        lazy_add_seq_no_last<Indices...>,
        lazy_add_seq<Indices...>>::type;
};

template <typename IndexList>
using add_seq_index_t = typename add_seq_index<IndexList>::type;
}  // namespace impl

/// @ingroup userver_containers
///
/// @brief MultiIndex LRU container
template <typename Value, typename IndexSpecifierList, typename Allocator = std::allocator<Value>>
class Container {
public:
    explicit Container(std::size_t max_size)
        : max_size_(max_size), buffer_size_(0)
    {}

    // for 2Q-caching, "buffer_size" argument represents size of FIFO buffer
    // total cache size is sum of max_size and buffer_size
    explicit Container(std::size_t max_size, std::size_t buffer_size)
        : max_size_(max_size), buffer_size_(buffer_size)
    {}

    template <typename... Args>
    bool emplace(Args&&... args) {
        if (buffer_size_ == 0) {
            return emplace_to_container(std::forward<Args>(args)...);
        } else {
            auto& seq_index = container_.template get<0>();
            auto& buffer_seq_index = fifo_buffer_.template get<0>();

            auto args_tuple = std::make_tuple(std::forward<Args>(args)...);
            
            auto result = std::apply([&](auto&&... args) {
                return seq_index.emplace_front(std::forward<decltype(args)>(args)...);
            }, args_tuple);
            
            auto buffer_result = std::apply([&](auto&&... args) {
                return buffer_seq_index.emplace_front(std::forward<decltype(args)>(args)...);
            }, args_tuple);

            if (!buffer_result.second && result.second) {
                buffer_seq_index.erase(buffer_result.first);
            } else if (!result.second) {
                buffer_seq_index.erase(buffer_result.first);
                seq_index.relocate(seq_index.begin(), result.first);
                if (seq_index.size() > max_size_) {
                    seq_index.pop_back();
                }
            } else {
                seq_index.erase(result.first);
                if (buffer_seq_index.size() > buffer_size_) {
                    buffer_seq_index.pop_back();
                }
            }
            return buffer_result.second || result.second;
        }
    }

    bool insert(const Value& value) { return emplace(value); }

    bool insert(Value&& value) { return emplace(std::move(value)); }

    template <typename Tag, typename Key>
    auto find(const Key& key) {
        auto& primary_index = container_.template get<Tag>();
        auto& primary_buffer_index = fifo_buffer_.template get<Tag>();
        auto it = primary_index.find(key);

        if (it != primary_index.end()) {
            auto& seq_index = container_.template get<0>();
            auto seq_it = container_.template project<0>(it);
            seq_index.relocate(seq_index.begin(), seq_it);
            return it;
        } else {
            auto buffer_it = primary_buffer_index.find(key);
            if (buffer_it != primary_buffer_index.end()) {
                this->insert(*buffer_it);
                return primary_index.find(key);
            }
        }

        return primary_index.end();
    }

    template <typename Tag, typename Key>
    bool contains(const Key& key) {
        return (this->template find<Tag, Key>(key) != container_.template get<Tag>().end());
    }

    template <typename Tag, typename Key>
    bool erase(const Key& key) {
        return (container_.template get<Tag>().erase(key) > 0) ||
               (fifo_buffer_.template get<Tag>().erase(key) > 0);
    }

    std::size_t size() const { return container_.size() + fifo_buffer_.size(); }
    bool empty() const { return container_.empty() && fifo_buffer_.empty(); }
    std::size_t capacity() const { return max_size_ + buffer_size_; }

    void set_capacity(std::size_t new_capacity) {
        max_size_ = new_capacity;
        auto& seq_index = container_.template get<0>();
        while (container_.size() > max_size_) {
            seq_index.pop_back();
        }
    }

    void clear() { container_.clear(); fifo_buffer_.clear(); }

    template <typename Tag>
    auto end() {
        return container_.template get<Tag>().end();
    }

private:
    using ExtendedIndexSpecifierList = impl::add_seq_index_t<IndexSpecifierList>;

    using BoostContainer = boost::multi_index::multi_index_container<Value, ExtendedIndexSpecifierList, Allocator>;
    // using BoostList = boost::multi_index::multi_index_container<
    //                                                 Value, 
    //                                                 boost::multi_index::indexed_by<boost::multi_index::sequenced<>>, 
    //                                                 Allocator>;
    BoostContainer container_;
    BoostContainer fifo_buffer_;
    std::size_t max_size_;
    std::size_t buffer_size_;

    template <typename... Args>
    bool emplace_to_container(Args&&... args) {
        auto& seq_index = container_.template get<0>();
        auto result = seq_index.emplace_front(std::forward<Args>(args)...);

        if (!result.second) {
            seq_index.relocate(seq_index.begin(), result.first);
        } else if (seq_index.size() > max_size_) {
            seq_index.pop_back();
        }
        return result.second;
    }
};
}  // namespace multi_index_lru

USERVER_NAMESPACE_END
