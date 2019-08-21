#pragma once

#include <type_traits>

#include <formats/parse/to.hpp>
#include <formats/serialize/to.hpp>
#include <utils/void_t.hpp>

/// @file formats/common/meta.hpp
/// @brief Metaprogramming helpers for converters detection.

namespace formats::common {

namespace impl {

template <class Value, class T, class = ::utils::void_t<>>
struct HasParseTo : std::false_type {};

template <class Value, class T>
struct HasParseTo<Value, T,
                  ::utils::void_t<decltype(
                      Parse(std::declval<const Value&>(), parse::To<T>{}))>>
    : std::true_type {};

template <class Value, class T, class = ::utils::void_t<>>
struct HasSerializeTo : std::false_type {};

template <class Value, class T>
struct HasSerializeTo<Value, T,
                      ::utils::void_t<decltype(Serialize(
                          std::declval<T>(), serialize::To<Value>{}))>>
    : std::true_type {};

template <class Value, class T, class = ::utils::void_t<>>
struct HasConvertTo : std::false_type {};

template <class Value, class T>
struct HasConvertTo<Value, T,
                    ::utils::void_t<decltype(
                        Convert(std::declval<const Value&>(), parse::To<T>{}))>>
    : std::true_type {};

template <typename Value, class = ::utils::void_t<>>
struct IsFormatValue : std::false_type {};

template <typename Value>
struct IsFormatValue<Value, ::utils::void_t<typename Value::ParseException>>
    : std::true_type {};

}  // namespace impl

/// Helper template variable to detect availability of the `Parse` overload.
template <class Value, class T>
constexpr inline bool kHasParseTo = impl::HasParseTo<Value, T>::value;

/// Helper template variable to detect availability of the `Serialize` overload.
template <class Value, class T>
constexpr inline bool kHasSerializeTo = impl::HasSerializeTo<Value, T>::value;

/// Helper template variable to detect availability of the `Convert` overload.
template <class Value, class T>
constexpr inline bool kHasConvertTo = impl::HasConvertTo<Value, T>::value;

/// Helper template variable to detect availability of the `Parse` overload.
template <class Value>
constexpr inline bool kIsFormatValue = impl::IsFormatValue<Value>::value;

}  // namespace formats::common
