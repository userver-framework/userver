#pragma once

#include <set>
#include <unordered_set>
#include <vector>

#include <boost/optional/optional.hpp>
#include <boost/variant/variant.hpp>

namespace formats::serialize {

template <typename Value, typename... Types>
Value Serialize(const boost::variant<Types>& value, To<Value>) {
  return boost::apply_visitor(
      [](const auto& item) { return Serialize(item, To<Value>{}); }, value);
}

template <typename Value, typename T>
Value Serialize(const boost::optional<T>& value, To<Value>) {
  if (!value) return {};

  return Serialize(*value, To<Value>{});
}

}  // namespace formats::serialize
