#pragma once

#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include <compiler/demangle.hpp>

namespace formats::parse {

namespace impl {

template <class T, class Value, typename Result>
void ParseVariantSingle(const Value& value, boost::optional<Result>& result) {
  if (result) {
    std::type_index old_type = result->type();
    try {
      value.template As<T>();
    } catch (const std::exception&) {
      return;
    }

    throw typename Value::ParseException(
        "Value of '" + value.GetPath() +
        "' is ambiguous, it is parseable into multiple variants of '" +
        compiler::GetTypeName<Result>() + "', at least '" +
        compiler::GetTypeName<T>() + "' and '" +
        compiler::GetTypeName(old_type) + "'");
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
  boost::optional<boost::variant<Types...>> result;
  (impl::ParseVariantSingle<Types>(value, result), ...);

  if (!result) {
    throw typename Value::ParseException(
        "Value of '" + value.GetPath() + "' cannot be parsed as " +
        compiler::GetTypeName<boost::variant<Types...>>());
  }

  return std::move(*result);
}

}  // namespace formats::parse
