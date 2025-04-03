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
[[noreturn]] void ThrowVariantParseException(std::string_view path) {
    throw ParseException(fmt::format("Value of '{}' cannot be parsed as {}", path, compiler::GetTypeName<Variant>()));
}

namespace impl {
template <class T, class Value, typename Result>
void ParseVariantSingle(const Value& value, std::optional<Result>& result) {
    if (result) {
        const auto old_type = std::visit([](const auto& v) -> std::type_index { return typeid(v); }, *result);
        try {
            value.template As<T>();
        } catch (const std::exception&) {
            return;
        }

        ThrowVariantAmbiguousParse<typename Value::ParseException, Result, T>(value.GetPath(), old_type);
    } else {
        // No result yet
        try {
            result = value.template As<T>();
        } catch (const std::exception&) {
        }
    }
}

}  // namespace impl

template <class Value, typename... Types>
auto Parse(const Value& value, formats::parse::To<std::variant<Types...>>) {
    std::optional<std::variant<decltype(Parse(std::declval<Value>(), To<Types>{}))...>> result;

    if constexpr (std::is_same_v<Value, formats::json::Value>) {
        Value value2{value};
        value2.DropRootPath();
        (impl::ParseVariantSingle<Types>(value2, result), ...);
    } else {
        (impl::ParseVariantSingle<Types>(value, result), ...);
    }

    if (!result) {
        ThrowVariantParseException<typename Value::ParseException, std::variant<Types...>>(value.GetPath());
    }

    return std::move(*result);
}

}  // namespace formats::parse

USERVER_NAMESPACE_END
