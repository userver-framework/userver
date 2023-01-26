#pragma once

/// @file userver/utils/meta.hpp
/// @brief Metaprogramming, template variables and concepts

#include <iosfwd>
#include <iterator>
#include <optional>
#include <type_traits>
#include <vector>

#include <userver/utils/void_t.hpp>

USERVER_NAMESPACE_BEGIN

namespace meta {

/// @{
/// @brief Detection idiom
///
/// To use, define a templated type alias (a "trait"), which for some type
/// either is correct and produces ("detects") some result type,
/// or is SFINAE-d out. Example:
/// @code
/// template <typename T>
/// using HasValueType = typename T::ValueType;
/// @endcode
struct NotDetected {};

namespace impl {

template <typename Default, typename AlwaysVoid,
          template <typename...> typename Trait, typename... Args>
struct Detector {
  using type = Default;
};

template <typename Default, template <typename...> typename Trait,
          typename... Args>
struct Detector<Default, utils::void_t<Trait<Args...>>, Trait, Args...> {
  using type = Trait<Args...>;
};

}  // namespace impl

/// Checks whether a trait is correct for the given template args
template <template <typename...> typename Trait, typename... Args>
inline constexpr bool kIsDetected = !std::is_same_v<
    typename impl::Detector<NotDetected, void, Trait, Args...>::type,
    NotDetected>;

/// Produces the result type of a trait, or NotDetected if it's incorrect
/// for the given template args
template <template <typename...> typename Trait, typename... Args>
using DetectedType =
    typename impl::Detector<NotDetected, void, Trait, Args...>::type;

/// Produces the result type of a trait, or Default if it's incorrect
/// for the given template args
template <typename Default, template <typename...> typename Trait,
          typename... Args>
using DetectedOr = typename impl::Detector<Default, void, Trait, Args...>::type;
/// @}

template <typename T, typename U>
using ExpectSame = std::enable_if_t<std::is_same_v<T, U>>;

namespace impl {

using std::begin;
using std::end;

template <template <typename...> typename Template, typename T>
struct IsInstantiationOf : std::false_type {};

template <template <typename...> typename Template, typename... Args>
struct IsInstantiationOf<Template, Template<Args...>> : std::true_type {};

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
using IsOstreamWritable =
    ExpectSame<decltype(std::declval<std::ostream&>()
                        << std::declval<const std::remove_reference_t<T>&>()),
               std::ostream&>;

template <typename T, typename U>
using IsEqualityComparable =
    ExpectSame<decltype(std::declval<const T&>() == std::declval<const U&>()),
               bool>;

template <typename T>
using IsStdHashable =
    ExpectSame<decltype(std::hash<T>{}(std::declval<const T&>())), size_t>;

template <typename T>
using IsSizable = decltype(std::size(std::declval<T>()));

template <typename T>
using ReserveResult = decltype(std::declval<T&>().reserve(1));

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

template <template <typename...> typename Template, typename... Args>
inline constexpr bool kIsInstantiationOf =
    impl::IsInstantiationOf<Template, Args...>::value;

template <typename T>
inline constexpr bool kIsVector = kIsInstantiationOf<std::vector, T>;

template <typename T>
inline constexpr bool kIsRange = kIsDetected<impl::IsRange, T>;

template <typename T>
inline constexpr bool kIsMap =
    kIsDetected<impl::IsRange, T> && kIsDetected<impl::KeyType, T> &&
    kIsDetected<impl::MappedType, T>;

template <typename T>
using MapKeyType = DetectedType<impl::KeyType, T>;

template <typename T>
using MapValueType = DetectedType<impl::MappedType, T>;

template <typename T>
using RangeValueType = DetectedType<impl::RangeValueType, T>;

template <typename T>
inline constexpr bool kIsRecursiveRange =
    std::is_same_v<DetectedType<RangeValueType, T>, T>;

template <typename T>
inline constexpr bool kIsOptional = kIsInstantiationOf<std::optional, T>;

/// Returns `true` if the type is a fundamental character type.
/// `signed char` and `unsigned char` are not character types.
template <typename T>
inline constexpr bool kIsCharacter =
    std::is_same_v<T, char> || std::is_same_v<T, wchar_t> ||
    std::is_same_v<T, char16_t> || std::is_same_v<T, char32_t>;

/// Returns `true` if the type is a true integer type (not `*char*` or `bool`)
/// `signed char` and `unsigned char` are integer types
template <typename T>
inline constexpr bool kIsInteger =
    std::is_integral_v<T> && !kIsCharacter<T> && !std::is_same_v<T, bool>;

template <typename T>
inline constexpr bool kIsOstreamWritable =
    kIsDetected<impl::IsOstreamWritable, T>;

template <typename T, typename U = T>
inline constexpr bool kIsEqualityComparable =
    kIsDetected<impl::IsEqualityComparable, T, U>;

template <typename T>
inline constexpr bool kIsStdHashable =
    kIsDetected<impl::IsStdHashable, T> && kIsEqualityComparable<T>;

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
