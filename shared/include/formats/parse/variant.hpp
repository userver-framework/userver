#pragma once

/// @file formats/parse/variant.hpp
/// @brief Ineffective but generic parser for std::variant type
///
/// Parsing is performed for each of the N alternatives in variant, N-1
/// exceptions is thrown and catched during the parsing.
///
/// @ingroup userver_formats_parse

#include <optional>
#include <typeinfo>
#include <variant>

#include <compiler/demangle.hpp>
#include <formats/parse/to.hpp>

namespace formats::parse {

template <typename ParseException, typename Variant, typename TypeA>
[[noreturn]] void ThrowVariantAmbiguousParse(const std::string& path,
                                             std::type_index type_b) {
  throw ParseException(
      "Value of '" + path +
      "' is ambiguous, it is parseable into multiple variants of '" +
      compiler::GetTypeName<Variant>() + "', at least '" +
      compiler::GetTypeName<TypeA>() + "' and '" +
      compiler::GetTypeName(type_b) + "'");
}

template <class ParseException, typename Variant>
[[noreturn]] void ThrowVariantParseException(const std::string& path) {
  throw ParseException("Value of '" + path + "' cannot be parsed as " +
                       compiler::GetTypeName<Variant>());
}

namespace impl {
template <class T, class Value, typename Result>
void ParseVariantSingle(const Value& value, std::optional<Result>& result) {
  if (result) {
    const auto old_type = std::visit(
        [](const auto& v) -> std::type_index { return typeid(v); }, *result);
    try {
      value.template As<T>();
    } catch (const std::exception&) {
      return;
    }

    ThrowVariantAmbiguousParse<typename Value::ParseException, Result, T>(
        value.GetPath(), old_type);
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
std::variant<Types...> Parse(const Value& value,
                             formats::parse::To<std::variant<Types...>>) {
  std::optional<std::variant<Types...>> result;
  (impl::ParseVariantSingle<Types>(value, result), ...);

  if (!result) {
    ThrowVariantParseException<typename Value::ParseException,
                               std::variant<Types...>>(value.GetPath());
  }

  return std::move(*result);
}

}  // namespace formats::parse
