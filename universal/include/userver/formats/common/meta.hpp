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

template <class Value, class T>
concept HasParse = requires(const Value& value) { Parse(value, parse::To<T>{}); };

template <class Value, class T>
concept HasSerialize = requires(const T& x) { Serialize(x, serialize::To<Value>{}); };

template <class Value, class T>
concept HasConvert = requires(const Value& value) { Convert(value, parse::To<T>{}); };

}  // namespace impl

/// Used in `Parse` overloads that are templated on `Value`, avoids clashing
/// with `Parse` from string
template <class Value>
concept kIsFormatValue = requires { typename Value::ParseException; };  // NOLINT(readability-identifier-naming)

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
