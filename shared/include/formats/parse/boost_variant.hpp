#pragma once

/// @file formats/parse/boost_variant.hpp
/// @brief Ineffective but generic parser for boost::variant type
///
/// Parsing is performed for each of the N alternatives in variant, N-1
/// exceptions is thrown and catched during the parsing.
///
/// @ingroup userver_formats_parse

#include <boost/variant/variant.hpp>

#include <compiler/demangle.hpp>
#include <formats/parse/variant.hpp>

namespace formats::parse {

namespace impl {
template <class T, class Value, typename Result>
void ParseBoostVariantSingle(const Value& value,
                             std::optional<Result>& result) {
  if (result) {
    std::type_index old_type = result->type();
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
boost::variant<Types...> Parse(const Value& value,
                               formats::parse::To<boost::variant<Types...>>) {
  std::optional<boost::variant<Types...>> result;
  (impl::ParseBoostVariantSingle<Types>(value, result), ...);

  if (!result) {
    ThrowVariantParseException<typename Value::ParseException,
                               boost::variant<Types...>>(value.GetPath());
  }

  return std::move(*result);
}

}  // namespace formats::parse
