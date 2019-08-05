#pragma once

#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/variant.hpp>

#include <formats/json/value_builder.hpp>
#include <formats/serialize/to.hpp>

namespace formats::serialize {

template <typename... Types>
formats::json::Value Serialize(const boost::variant<Types...>& value,
                               To<::formats::json::Value>) {
  return boost::apply_visitor(
      [](const auto& item) {
        return formats::json::ValueBuilder(item).ExtractValue();
      },
      value);
}

template <typename... Types>
json::Value Serialize(const boost::variant<Types...>& value, To<json::Value>) {
  return boost::apply_visitor(
      [](const auto& item) { return json::ValueBuilder(item).ExtractValue(); },
      value);
}

}  // namespace formats::serialize
