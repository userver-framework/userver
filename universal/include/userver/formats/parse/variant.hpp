#pragma once

/// @file userver/formats/parse/variant.hpp
/// @brief Ineffective but generic parser for std::variant type
///
/// Parsing is performed for each of the N alternatives in variant, N-1
/// exceptions is thrown and caught during the parsing.
///
/// @ingroup userver_universal userver_formats_parse

#include <optional>
#include <typeinfo>
#include <variant>

#include <fmt/format.h>

#include <userver/compiler/demangle.hpp>
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

template <class T, class Value, typename Result>
void ParseVariantSingle(const Value& value, std::optional<Result>& result, std::string& errors) {
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
            errors = fmt::format("{}\n* Failed to parse as {}: {}", errors, compiler::GetTypeName<T>(), e.what());
        }
    }
}

}  // namespace impl

template <class Value, typename... Types>
std::variant<decltype(Parse(std::declval<Value>(), To<Types>{}))...>
Parse(const Value& value, formats::parse::To<std::variant<Types...>>) {
    std::optional<std::variant<decltype(Parse(std::declval<Value>(), To<Types>{}))...>> result;

    std::string errors;

    if constexpr (std::is_same_v<Value, formats::json::Value>) {
        Value value2{value};
        value2.DropRootPath();
        (impl::ParseVariantSingle<Types>(value2, result, errors), ...);
    } else {
        (impl::ParseVariantSingle<Types>(value, result, errors), ...);
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
