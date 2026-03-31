#pragma once

#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>

#include <chrono>
#include <cstddef>
#include <tuple>
#include <utility>

USERVER_NAMESPACE_BEGIN

namespace multi_index_lru::impl {
template <typename T, typename = std::void_t<>>
inline constexpr bool is_mpl_na = false;

template <typename T>
inline constexpr bool is_mpl_na<T, std::void_t<decltype(std::declval<T>().~na())>> = true;

template <typename IndexType, typename... Indices>
struct lazy_add_index {
    using type = boost::multi_index::indexed_by<IndexType, Indices...>;
};

template <typename IndexType, typename... Indices>
struct lazy_add_index_no_last {
private:
    template <std::size_t... I>
    static auto MakeWithoutLast(std::index_sequence<I...>) {
        using Tuple = std::tuple<Indices...>;
        return boost::multi_index::indexed_by<IndexType, std::tuple_element_t<I, Tuple>...>{};
    }

public:
    using type = decltype(MakeWithoutLast(std::make_index_sequence<sizeof...(Indices) - 1>{}));
};

template <typename IndexType, typename IndexList>
struct add_index {};

template <typename IndexType, typename... Indices>
struct add_index<IndexType, boost::multi_index::indexed_by<Indices...>> {
    using LastType = decltype((Indices{}, ...));

    using type = typename std::conditional_t<
        is_mpl_na<LastType>,
        lazy_add_index_no_last<IndexType, Indices...>,
        lazy_add_index<IndexType, Indices...>>::type;
};

template <typename IndexType, typename IndexList>
using add_index_t = typename add_index<IndexType, IndexList>::type;

template <typename Value>
struct TimestampedValue {
    Value value;
    mutable std::chrono::steady_clock::time_point last_accessed;

    TimestampedValue() = default;

    explicit TimestampedValue(const Value& val)
        : value(val),
          last_accessed(std::chrono::steady_clock::now())
    {}

    explicit TimestampedValue(Value&& val)
        : value(std::move(val)),
          last_accessed(std::chrono::steady_clock::now())
    {}

    operator Value&() noexcept { return value; }
    operator const Value&() const noexcept { return value; }

    Value& get() noexcept { return value; }
    const Value& get() const noexcept { return value; }
};

template <typename Iterator>
class IteratorToValue : public Iterator {
public:
    using Iterator::Iterator;

    explicit IteratorToValue(Iterator iter)
        : Iterator(std::move(iter))
    {}

    decltype(auto) operator->() noexcept { return std::addressof((**this).value); }

    decltype(auto) operator->() const noexcept { return std::addressof((**this).value); }
};

}  // namespace multi_index_lru::impl

USERVER_NAMESPACE_END
