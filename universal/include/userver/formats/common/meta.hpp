#pragma once

/// @file userver/formats/common/meta.hpp
/// @brief Metaprogramming helpers for converters detection.
/// @ingroup userver_universal

#include <type_traits>

#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/to.hpp>
#include <userver/utils/meta_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::common {

namespace impl {

/// `kHasX` are only intended for internal diagnostic use!
///
/// `formats` doesn't support SFINAE, so e.g. `kHasParse` can return `true`
/// while a usage of `Parse` will fail to compile.

template <typename Value, typename T>
using HasParse = decltype(Parse(std::declval<const Value&>(), parse::To<T>{}));

template <typename Value, typename T>
using HasSerialize =
    decltype(Serialize(std::declval<const T&>(), serialize::To<Value>{}));

template <typename Value, typename T>
using HasConvert =
    decltype(Convert(std::declval<const Value&>(), parse::To<T>{}));

template <typename Value>
using IsFormatValue = typename Value::ParseException;

template <class Value, class T>
constexpr inline bool kHasParse = meta::kIsDetected<HasParse, Value, T>;

template <class Value, class T>
constexpr inline bool kHasSerialize = meta::kIsDetected<HasSerialize, Value, T>;

template <class Value, class T>
constexpr inline bool kHasConvert = meta::kIsDetected<HasConvert, Value, T>;

}  // namespace impl

/// Used in `Parse` overloads that are templated on `Value`, avoids clashing
/// with `Parse` from string
template <class Value>
constexpr inline bool kIsFormatValue =
    meta::kIsDetected<impl::IsFormatValue, Value>;

// Unwraps a transient type - tag types, for which ADL-found `Parse` returns
// another type, not the type specified in `formats::parse::To`. For example,
// there can be a `IntegerWithMin<42>` type that checks that the value contains
// a number `>= 42`, then parses and returns an `int`.
//
// For a normal type T, just returns the type T itself.
template <typename Value, typename T>
using ParseType = decltype(Parse(std::declval<Value>(), parse::To<T>()));

}  // namespace formats::common

USERVER_NAMESPACE_END
