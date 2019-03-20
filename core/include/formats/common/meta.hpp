#pragma once

#include <type_traits>
#include <utils/void_t.hpp>

#include <formats/parse/to.hpp>

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
}  // namespace impl

/// Helper template variable to detect availability of the
/// `FormatsParseCommon` overload.
template <class Value, class T>
constexpr inline bool kHasParseTo = impl::HasParseTo<Value, T>::value;

}  // namespace formats::common
