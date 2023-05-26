#pragma once

/// @file userver/utils/meta.hpp
/// @brief Metaprogramming, template variables and concepts

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
using IsRange = ExpectSame<std::decay_t<decltype(begin(std::declval<T&>()))>,
                           std::decay_t<decltype(end(std::declval<T&>()))>>;

template <typename T>
using IteratorType = std::enable_if_t<kIsDetected<IsRange, T>,
                                      decltype(begin(std::declval<T&>()))>;

template <typename T>
using RangeValueType =
    typename std::iterator_traits<DetectedType<IteratorType, T>>::value_type;

template <typename T>
using OstreamWriteResult =
    decltype(std::declval<std::ostream&>()
             << std::declval<const std::remove_reference_t<T>&>());

template <typename T, typename U>
using EqualityComparisonResult =
    decltype(std::declval<const T&>() == std::declval<const U&>());

template <typename T>
using StdHashResult = decltype(std::hash<T>{}(std::declval<const T&>()));

template <typename T>
using IsSizable = decltype(std::size(std::declval<T>()));

template <typename T>
using ReserveResult = decltype(std::declval<T&>().reserve(1));

template <typename T>
using AtResult =
    decltype(std::declval<const T&>().at(std::declval<typename T::key_type>()));

template <typename T>
using SubscriptOperatorResult =
    decltype(std::declval<T>()[std::declval<typename T::key_type>()]);

template <typename T>
using PushBackResult = decltype(std::declval<T&>().push_back({}));

template <typename T>
struct IsFixedSizeContainer : std::false_type {};

// Boost and std arrays
template <typename T, std::size_t Size,
          template <typename, std::size_t> typename Array>
struct IsFixedSizeContainer<Array<T, Size>>
    : std::bool_constant<sizeof(Array<T, Size>) == sizeof(T) * Size> {};

template <typename... Args>
constexpr bool IsSingleRange() {
  if constexpr (sizeof...(Args) == 1) {
    return kIsDetected<IsRange, Args...>;
  } else {
    return false;
  }
}

}  // namespace impl

template <typename T>
inline constexpr bool kIsVector = kIsInstantiationOf<std::vector, T>;

template <typename T>
inline constexpr bool kIsRange = kIsDetected<impl::IsRange, T>;

/// Returns true if T is an ordered or unordered map or multimap
template <typename T>
inline constexpr bool kIsMap =
    kIsDetected<impl::IsRange, T> && kIsDetected<impl::KeyType, T> &&
    kIsDetected<impl::MappedType, T>;

/// Returns true if T is a map (but not a multimap!)
template <typename T>
inline constexpr bool kIsUniqueMap =
    kIsMap<T> && kIsDetected<impl::SubscriptOperatorResult,
                             T>;  // no operator[] in multimaps

template <typename T>
using MapKeyType = DetectedType<impl::KeyType, T>;

template <typename T>
using MapValueType = DetectedType<impl::MappedType, T>;

template <typename T>
using RangeValueType = DetectedType<impl::RangeValueType, T>;

template <typename T>
inline constexpr bool kIsRecursiveRange =
    std::is_same_v<DetectedType<impl::RangeValueType, T>, T>;

template <typename T>
inline constexpr bool kIsOptional = kIsInstantiationOf<std::optional, T>;

template <typename T>
inline constexpr bool kIsOstreamWritable =
    std::is_same_v<DetectedType<impl::OstreamWriteResult, T>, std::ostream&>;

template <typename T, typename U = T>
inline constexpr bool kIsEqualityComparable =
    std::is_same_v<DetectedType<impl::EqualityComparisonResult, T, U>, bool>;

template <typename T>
inline constexpr bool kIsStdHashable =
    std::is_same_v<DetectedType<impl::StdHashResult, T>, std::size_t> &&
    kIsEqualityComparable<T>;

/// @brief  Check if std::size is applicable to container
template <typename T>
inline constexpr bool kIsSizable = kIsDetected<impl::IsSizable, T>;

/// @brief Check if a container has `reserve`
template <typename T>
inline constexpr bool kIsReservable = kIsDetected<impl::ReserveResult, T>;

/// @brief Check if a container has 'push_back'
template <typename T>
inline constexpr bool kIsPushBackable = kIsDetected<impl::PushBackResult, T>;

/// @brief Check if a container has fixed size (e.g. std::array)
template <typename T>
inline constexpr bool kIsFixedSizeContainer =
    impl::IsFixedSizeContainer<T>::value;

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

}  // namespace meta

USERVER_NAMESPACE_END
