#pragma once

/// @file userver/utils/meta.hpp
/// @brief Metaprogramming, template variables and concepts
/// @ingroup userver_universal

#include <concepts>
#include <iosfwd>
#include <iterator>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

#include <userver/compiler/impl/nodebug.hpp>
#include <userver/utils/meta_light.hpp>

USERVER_NAMESPACE_BEGIN

/// @brief Metaprogramming utilities, concepts, and type traits helpers.
namespace meta {

namespace impl {

using std::begin;
using std::end;

template <typename T>
concept IsRange = requires(T& t) {
    {
        begin(t)
    };
    {
        end(t)
    };
};

template <IsRange T>
using IteratorType USERVER_IMPL_NODEBUG = decltype(begin(std::declval<T&>()));

template <typename T>
using RangeValueType USERVER_IMPL_NODEBUG = typename std::iterator_traits<IteratorType<T>>::value_type;

template <typename T>
struct IsFixedSizeContainer : std::false_type {};

// Boost and std arrays
template <typename T, std::size_t Size, template <typename, std::size_t> typename Array>
struct IsFixedSizeContainer<Array<T, Size>> : std::bool_constant<sizeof(Array<T, Size>) == sizeof(T) * Size> {};

template <typename... Args>
concept IsSingleRange = (sizeof...(Args) == 1) && (impl::IsRange<Args> && ...);

template <typename Arg, typename OldT>
concept IsAllocatorFor = requires { typename Arg::value_type; } && std::same_as<typename Arg::value_type, OldT>;

// A policy parameterised by the container's value type — std::less<OldT>,
// std::hash<OldT>, std::equal_to<OldT> — takes the new type in its place.
template <typename Arg, typename OldT, typename T>
struct SubstituteFirst {
    using type = Arg;
};

template <template <typename...> typename Tmpl, typename OldT, typename T, typename... Rest>
struct SubstituteFirst<Tmpl<OldT, Rest...>, OldT, T> {
    using type = Tmpl<T, Rest...>;
};

// An allocator is rebound by std::allocator_traits rather than by substitution,
// because an allocator need not be a template over its value type at all; a
// policy is substituted, and anything else is left alone.
template <bool IsAllocator, typename Arg, typename OldT, typename T>
struct RebindArgImpl {
    using type = typename SubstituteFirst<Arg, OldT, T>::type;
};

template <typename Arg, typename OldT, typename T>
struct RebindArgImpl<true, Arg, OldT, T> {
    using type = typename std::allocator_traits<Arg>::template rebind_alloc<T>;
};

template <typename Arg, typename OldT, typename T>
using RebindArg = typename RebindArgImpl<IsAllocatorFor<Arg, OldT>, Arg, OldT, T>::type;

template <typename Container, typename T>
struct RebindContainer;

template <template <typename...> typename Container, typename OldT, typename... Rest, typename T>
struct RebindContainer<Container<OldT, Rest...>, T> {
    using type = Container<T, RebindArg<Rest, OldT, T>...>;
};

template <template <typename, auto...> typename Container, typename OldT, typename T, auto... Args>
struct RebindContainer<Container<OldT, Args...>, T> {
    using type = Container<T, Args...>;
};

}  // namespace impl

/// @warning Use std::ranges::range instead of this concept, except possibly in common headers
/// where compilation time is a concern.
template <typename T>
concept IsRange = impl::IsRange<T>;

/// Returns true if T is an ordered or unordered map or multimap
template <typename T>
concept IsMap = IsRange<T> && requires {
    typename T::key_type;
    typename T::mapped_type;
};

/// Returns true if T is a map (but not a multimap!)
template <typename T>
concept IsUniqueMap = IsMap<T> && requires(T& map, typename T::key_type key) {
    map[key];  // no operator[] in multimaps
};

template <IsMap T>
using MapKeyType = typename T::key_type;

template <IsMap T>
using MapValueType = typename T::mapped_type;

/// @warning Use std::ranges::range_value_t instead of this type, except possibly in common headers
/// where compilation time is a concern.
template <IsRange T>
using RangeValueType = impl::RangeValueType<T>;

template <typename T>
concept IsRecursiveRange = IsRange<T> && std::same_as<impl::RangeValueType<T>, T>;

template <typename T>
concept IsOptional = IsInstantiationOf<T, std::optional>;

template <typename T>
concept IsOstreamWritable = requires(std::ostream& os, const std::remove_reference_t<T>& val) {
    {
        os << val
    } -> std::same_as<std::ostream&>;
};

template <typename T>
concept IsStdHashable = requires(const T& val) {
    {
        std::hash<T>{}(val)
    } -> std::same_as<std::size_t>;
} && std::equality_comparable<T>;

/// @brief Check if std::size is applicable to container
/// @warning Use std::ranges::sized_range instead of this concept, except possibly in common headers
/// where compilation time is a concern.
template <typename T>
concept IsSizable = IsRange<T> && requires(T value) { std::size(value); };

/// @brief Check if a container has `reserve`
template <typename T>
concept IsReservable = IsSizable<T> && requires(T value) { value.reserve(1); };

/// @brief Check if a container has 'push_back'
template <typename T>
concept IsPushBackable = IsRange<T> && requires(T value, RangeValueType<T> element) {
    value.push_back(std::move(element));
};

/// @brief Check if a container has fixed size (e.g. `std::array`)
template <typename T>
concept IsFixedSizeContainer = IsRange<T> && impl::IsFixedSizeContainer<T>::value;

/// @brief The same container template holding `T`, with its allocator rebound to `T`.
///
/// For an allocator-aware container the allocator is **rebound** rather than carried over:
/// `std::vector<U, Alloc>`'s `Alloc` allocates `U`, and a container of `T` needs one that
/// allocates `T`. `std::allocator_traits` is what knows how to ask, so an allocator that
/// provides its own `rebind`, or that is not a plain `Alloc<U>`, is handled too.
///
/// Every other type argument is carried across the same way: a policy parameterised by the
/// value type — `std::less<U>`, `std::hash<U>`, `std::equal_to<U>` — takes `T` in its place,
/// so an associative container keeps a comparator that matches what it now holds, and
/// anything unrelated is left as it is.
///
/// For a container parameterised by values rather than types — `std::array<U, N>` — the
/// values are kept: `RebindContainer<std::array<U, N>, T>` is `std::array<T, N>`.
template <typename Container, typename T>
using RebindContainer = typename impl::RebindContainer<Container, T>::type;

template <typename T>
concept IsVectorLike = IsRange<T> && std::default_initializable<T> && IsReservable<T> && IsPushBackable<T>;

/// @brief Returns default inserter for a container
template <typename T>
auto Inserter(T& container) {
    if constexpr (IsPushBackable<T>) {
        return std::back_inserter(container);
    } else if constexpr (IsFixedSizeContainer<T>) {
        return container.begin();
    } else {
        return std::inserter(container, container.end());
    }
}

}  // namespace meta

USERVER_NAMESPACE_END
