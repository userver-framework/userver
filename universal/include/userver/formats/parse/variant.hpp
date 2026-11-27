#pragma once

/// @file userver/formats/parse/variant.hpp
/// @brief Generic parser for std::variant type
///
/// Parsing is performed for each of the N alternatives in variant, and an
/// exception is thrown and caught for each alternative that fails to parse.
/// Alternatives that @ref formats::parse::MayParseAs rejects are parsed only
/// if no alternative fits, to report their errors.
///
/// @ingroup userver_universal userver_formats_parse

#include <array>
#include <cstddef>
#include <exception>
#include <optional>
#include <string>
#include <typeinfo>
#include <utility>
#include <variant>

#include <fmt/format.h>

#include <userver/compiler/demangle.hpp>
#include <userver/formats/parse/may_parse.hpp>
#include <userver/formats/parse/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json {
class Value;
}

namespace formats::parse {

template <typename ParseException, typename Variant, typename TypeA>
[[noreturn]] void ThrowVariantAmbiguousParse(std::string_view path, std::type_index type_b) {
    throw ParseException(fmt::format(
        "Value of '{}' is ambiguous, it is parseable into multiple variants of '{}', at least '{}' and '{}'",
        path,
        compiler::GetTypeName<Variant>(),
        compiler::GetTypeName<TypeA>(),
        compiler::GetTypeName(type_b)
    ));
}

template <class ParseException, typename Variant>
[[noreturn]] void ThrowVariantParseException(std::string_view path, std::string_view errors) {
    throw ParseException(fmt::format(
        "Value of '{}' cannot be parsed as {}. Issues during parsing follow: {}",
        path,
        compiler::GetTypeName<Variant>(),
        errors
    ));
}

namespace impl {

template <class T>
std::string FormatVariantParseError(const std::exception& e) {
    return fmt::format("\n* Failed to parse as {}: {}", compiler::GetTypeName<T>(), e.what());
}

template <class T, class Value, typename Result>
void ParseVariantSingle(const Value& value, std::optional<Result>& result, std::string& error) {
    if (!formats::parse::MayParseAs<T>(value)) {
        return;
    }

    if (result) {
        const auto old_type = std::visit([](const auto& v) -> std::type_index { return typeid(v); }, *result);
        try {
            value.template As<T>();
        } catch (const std::exception&) {
            return;
        }

        using ParseException = typename Value::ParseException;
        formats::parse::ThrowVariantAmbiguousParse<ParseException, Result, T>(value.GetPath(), old_type);
    } else {
        // No result yet
        try {
            result = value.template As<T>();
        } catch (const std::exception& e) {
            error = impl::FormatVariantParseError<T>(e);
        }
    }
}

template <class T, class Value>
void ParseRejectedVariantSingle(const Value& value, std::string& error) {
    if (formats::parse::MayParseAs<T>(value)) {
        return;
    }

    try {
        value.template As<T>();
    } catch (const std::exception& e) {
        error = impl::FormatVariantParseError<T>(e);
    }
}

template <typename... Types, class Value, typename Result, std::size_t... Indices>
std::string
ParseVariantAlternatives(const Value& value, std::optional<Result>& result, std::index_sequence<Indices...>) {
    std::array<std::string, sizeof...(Types)> errors;
    (impl::ParseVariantSingle<Types>(value, result, errors[Indices]), ...);
    if (result) {
        return {};
    }

    (impl::ParseRejectedVariantSingle<Types>(value, errors[Indices]), ...);
    return (std::string{} + ... + errors[Indices]);
}

}  // namespace impl

template <class Value, typename... Types>
std::variant<decltype(Parse(std::declval<Value>(), To<Types>{}))...>
Parse(const Value& value, formats::parse::To<std::variant<Types...>>) {
    using Result = std::variant<decltype(Parse(std::declval<Value>(), To<Types>{}))...>;
    std::optional<Result> result;

    std::string errors;

    if constexpr (std::is_same_v<Value, formats::json::Value>) {
        Value value2{value};
        value2.DropRootPath();
        errors = impl::ParseVariantAlternatives<Types...>(value2, result, std::index_sequence_for<Types...>{});
    } else {
        errors = impl::ParseVariantAlternatives<Types...>(value, result, std::index_sequence_for<Types...>{});
    }

    if (!result) {
        formats::parse::ThrowVariantParseException<
            typename Value::ParseException,
            std::variant<Types...>>(value.GetPath(), errors);
    }

    return std::move(*result);
}

}  // namespace formats::parse

USERVER_NAMESPACE_END
