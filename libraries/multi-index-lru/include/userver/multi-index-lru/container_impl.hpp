#pragma once

#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>

#include <cstddef>
#include <tuple>
#include <utility>
#include <chrono>

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


template<typename Value>
struct TimestampedValue : public Value {
    std::chrono::steady_clock::time_point last_accessed;

    TimestampedValue() = default;

    explicit TimestampedValue(const Value& val)
        : Value(val),
          last_accessed(std::chrono::steady_clock::now()) {}

    explicit TimestampedValue(Value&& val)
        : Value(std::move(val)),
          last_accessed(std::chrono::steady_clock::now()) {}
};
} // namespace impl
} // namespace multi_index_lru

USERVER_NAMESPACE_END