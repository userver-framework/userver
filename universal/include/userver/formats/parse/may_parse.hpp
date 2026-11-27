#pragma once

/// @file userver/formats/parse/may_parse.hpp
/// @brief Cheap checks that tell when parsing a value is certain to fail, for
/// skipping alternatives without throwing an exception.
///
/// @ingroup userver_universal userver_formats_parse

#include <exception>
#include <string>
#include <type_traits>

#include <userver/formats/parse/to.hpp>
#include <userver/utils/meta_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json {
class Value;
}

namespace formats::parse {

namespace impl {

bool IsJsonNumber(const formats::json::Value& value);

}  // namespace impl

/// @brief Fallback for types without a check: returns `true`, so the value has to
/// be parsed.
template <typename Value, typename T>
bool MayParseAs(const Value&, To<T>) noexcept {
    return true;
}

/// @brief Built-in check for `std::string`: returns `false` if the JSON value is
/// not a string.
bool MayParseAs(const formats::json::Value& value, To<std::string>);

/// @brief Built-in check for `bool`: returns `false` if the JSON value is not a
/// boolean.
bool MayParseAs(const formats::json::Value& value, To<bool>);

/// @brief Built-in check for integer and floating point types: returns `false`
/// if the JSON value is not a number.
template <typename T>
requires(meta::IsInteger<T> || std::is_floating_point_v<T>)
bool MayParseAs(const formats::json::Value& value, To<T>) {
    return impl::IsJsonNumber(value);
}

/// @brief Cheap necessary condition for `value.As<T>()` to succeed.
///
/// Calls `MayParseAs(value, formats::parse::To<T>{})`: a built-in overload from
/// this header, an overload found by ADL in the namespace of `T`, usually
/// declared next to the `Parse` function of `T`, or the fallback that returns
/// `true`. An overload must return `false` only if `value.As<T>()` is certain to
/// throw. It should not throw itself: an exception derived from `std::exception`
/// is treated as `true`, and the value is parsed as usual.
///
/// An overload should look only at the value itself, for example at its type,
/// the presence of required members or the enum value, and should not check
/// nested values: a recursive check repeats the work of parsing, especially
/// for nested `std::variant`.
///
/// @returns `false` if `value.As<T>()` is certain to throw; `true` if nothing is
/// known and the value has to be parsed.
template <typename T, typename Value>
bool MayParseAs(const Value& value) {
    try {
        return MayParseAs(value, To<T>{});
    } catch (const std::exception&) {
        return true;
    }
}

}  // namespace formats::parse

USERVER_NAMESPACE_END
