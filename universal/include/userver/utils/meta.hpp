#pragma once

/// @file userver/utils/meta.hpp
/// @brief Metaprogramming, template variables and concepts
/// @ingroup userver_universal

#include <concepts>
#include <iosfwd>
#include <iterator>
#include <optional>
#include <type_traits>
#include <vector>

#include <userver/utils/meta_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace meta {

namespace impl {

using std::begin;
using std::end;

template <typename T>
using KeyType = typename T::key_type;

template <typename T>
using MappedType = typename T::mapped_type;

template <typename T>
requires std::is_same_v<
             std::decay_t<decltype(begin(std::declval<T&>()))>,
             std::decay_t<decltype(end(std::declval<T&>()))>>
using IsRange = void;

template <typename T>
requires IsDetected<IsRange, T>
using IteratorType = decltype(begin(std::declval<T&>()));

template <typename T>
using RangeValueType = typename std::iterator_traits<DetectedType<IteratorType, T>>::value_type;

template <typename T>
using OstreamWriteResult = decltype(std::declval<std::ostream&>() << std::declval<const std::remove_reference_t<T>&>());

template <typename T>
using StdHashResult = decltype(std::hash<T>{}(std::declval<const T&>()));

template <typename T>
using AtResult = decltype(std::declval<const T&>().at(std::declval<typename T::key_type>()));

template <typename T>
using SubscriptOperatorResult = decltype(std::declval<T>()[std::declval<typename T::key_type>()]);

template <typename T>
struct IsFixedSizeContainer : std::false_type {};

// Boost and std arrays
template <typename T, std::size_t Size, template <typename, std::size_t> typename Array>
struct IsFixedSizeContainer<Array<T, Size>> : std::bool_constant<sizeof(Array<T, Size>) == sizeof(T) * Size> {};

template <typename... Args>
constexpr bool IsSingleRange() {
    if constexpr (sizeof...(Args) == 1) {
        return IsDetected<IsRange, Args...>;
    } else {
        return false;
    }
}

}  // namespace impl

// NOLINTBEGIN(readability-identifier-naming)

template <typename T>
concept kIsVector = kIsInstantiationOf<std::vector, T>;

template <typename T>
concept kIsRange = IsDetected<impl::IsRange, T>;

/// Returns true if T is an ordered or unordered map or multimap
template <typename T>
concept kIsMap = IsDetected<impl::IsRange, T> && IsDetected<impl::KeyType, T> && IsDetected<impl::MappedType, T>;

/// Returns true if T is a map (but not a multimap!)
template <typename T>
concept kIsUniqueMap =
    kIsMap<T> &&
    IsDetected<
        impl::SubscriptOperatorResult,
        T>;  // no operator[] in multimaps

template <typename T>
using MapKeyType = DetectedType<impl::KeyType, T>;

template <typename T>
using MapValueType = DetectedType<impl::MappedType, T>;

template <typename T>
using RangeValueType = DetectedType<impl::RangeValueType, T>;

template <typename T>
concept kIsRecursiveRange = std::is_same_v<DetectedType<impl::RangeValueType, T>, T>;

template <typename T>
concept kIsIterator = requires { typename std::iterator_traits<T>::iterator_category; };

template <typename T>
concept kIsOptional = kIsInstantiationOf<std::optional, T>;

template <typename T>
concept kIsOstreamWritable = std::is_same_v<DetectedType<impl::OstreamWriteResult, T>, std::ostream&>;

template <typename T, typename U = T>
concept kIsEqualityComparable = std::equality_comparable_with<T, U>;

template <typename T>
concept kIsStdHashable = std::is_same_v<DetectedType<impl::StdHashResult, T>, std::size_t> && kIsEqualityComparable<T>;

/// @brief  Check if std::size is applicable to container
template <typename T>
concept kIsSizable = requires(T value) { std::size(value); };

/// @brief Check if a container has `reserve`
template <typename T>
concept kIsReservable = requires(T value) { value.reserve(1); };

/// @brief Check if a container has 'push_back'
template <typename T>
concept kIsPushBackable = requires(T value) { value.push_back({}); };

/// @brief Check if a container has fixed size (e.g. std::array)
template <typename T>
concept kIsFixedSizeContainer = impl::IsFixedSizeContainer<T>::value;

/// @brief Returns default inserter for a container
template <typename T>
auto Inserter(T& container) {
    if constexpr (kIsPushBackable<T>) {
        return std::back_inserter(container);
    } else if constexpr (kIsFixedSizeContainer<T>) {
        return container.begin();
    } else {
        return std::inserter(container, container.end());
    }
}

// NOLINTEND(readability-identifier-naming)

}  // namespace meta

USERVER_NAMESPACE_END
