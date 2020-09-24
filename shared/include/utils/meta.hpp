#pragma once

#include <array>
#include <functional>
#include <iosfwd>
#include <iterator>
#include <optional>
#include <type_traits>
#include <vector>

namespace meta {

namespace impl {

template <template <typename...> typename Template, typename T>
struct IsInstantiationOf : std::false_type {};

template <template <typename...> typename Template, typename... Args>
struct IsInstantiationOf<Template, Template<Args...>> : std::true_type {};

template <typename T, typename = void>
struct HasKeyType : std::false_type {};

template <typename T>
struct HasKeyType<T, std::void_t<typename T::key_type>> : std::true_type {};

template <typename T, typename = void>
struct HasMappedType : std::false_type {};

template <typename T>
struct HasMappedType<T, std::void_t<typename T::mapped_type>> : std::true_type {
};

template <typename T, typename = void>
struct IsRange : std::false_type {};

template <typename T>
struct IsRange<T, std::void_t<decltype(std::begin(std::declval<T&>())),
                              decltype(std::end(std::declval<T&>()))>>
    : std::true_type {};

template <typename T>
using IteratorType = decltype(std::begin(std::declval<T&>()));

template <typename T>
struct RangeValueTypeImpl {
  using type = typename std::iterator_traits<IteratorType<T>>::value_type;
};

template <typename T, typename = void>
struct IsRecursiveRange : std::false_type {};

template <typename T>
struct IsRecursiveRange<T, std::enable_if_t<IsRange<T>::value>>
    : std::integral_constant<
          bool, std::is_same_v<typename RangeValueTypeImpl<T>::type, T>> {};

template <typename T>
struct IsOptional : IsInstantiationOf<std::optional, T> {};

template <typename T>
struct IsCharacter
    : std::integral_constant<bool, std::is_same_v<T, char> ||
                                       std::is_same_v<T, wchar_t> ||
                                       std::is_same_v<T, char16_t> ||
                                       std::is_same_v<T, char32_t>> {};

template <typename T>
struct IsInteger : std::integral_constant<bool, std::is_integral_v<T> &&
                                                    !IsCharacter<T>::value &&
                                                    !std::is_same_v<T, bool>> {
};

template <typename T, typename = void>
struct IsOstreamWritable : std::false_type {};

template <typename T>
struct IsOstreamWritable<
    T,
    std::void_t<decltype(std::declval<std::ostream&>()
                         << std::declval<const std::remove_reference_t<T>&>())>>
    : std::true_type {};

template <typename T, typename U, typename = void>
struct IsEqualityComparable : std::false_type {};

template <typename T, typename U>
struct IsEqualityComparable<
    T, U, std::void_t<decltype(std::declval<T&>() == std::declval<U&>())>>
    : std::true_type {};

template <typename T, typename = void>
struct IsStdHashable : std::false_type {};

template <typename T>
struct IsStdHashable<
    T, std::void_t<decltype(std::hash<T>{}(std::declval<const T&>()))>>
    : std::true_type {};

}  // namespace impl

template <typename T>
using IsBool = std::is_same<T, bool>;

template <typename T>
inline constexpr bool kIsBool = IsBool<T>::value;

template <template <typename...> typename Template, typename... Args>
inline constexpr bool kIsInstantiationOf =
    impl::IsInstantiationOf<Template, Args...>::value;

template <typename T>
inline constexpr bool kIsVector = kIsInstantiationOf<std::vector, T>;

template <typename T>
inline constexpr bool kIsRange = impl::IsRange<T>::value;

template <typename T>
struct IsMap
    : std::integral_constant<bool, kIsRange<T> && impl::HasKeyType<T>::value &&
                                       impl::HasMappedType<T>::value> {};

template <typename T>
inline constexpr bool kIsMap = IsMap<T>::value;

template <typename T>
using RangeValueType = typename impl::RangeValueTypeImpl<T>::type;

template <typename T>
inline constexpr bool kIsRecursiveRange = impl::IsRecursiveRange<T>::value;

template <typename T>
inline constexpr bool kIsOptional = impl::IsOptional<T>::value;

/// Returns `true` if the type is a fundamental character type.
/// `signed char` and `unsigned char` are not character types.
template <typename T>
inline constexpr bool kIsCharacter = impl::IsCharacter<T>::value;

/// Returns `true` if the type is a true integer type (not `*char*` or `bool`)
/// `signed char` and `unsigned char` are integer types
template <typename T>
inline constexpr bool kIsInteger = impl::IsInteger<T>::value;

template <typename T>
inline constexpr bool kIsOstreamWritable = impl::IsOstreamWritable<T>::value;

template <typename T, typename U = T>
inline constexpr bool kIsEqualityComparable =
    impl::IsEqualityComparable<T, U>::value;

template <typename T>
inline constexpr bool kIsStdHashable =
    impl::IsStdHashable<T>::value&& kIsEqualityComparable<T>;

}  // namespace meta
