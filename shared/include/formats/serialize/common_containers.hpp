#pragma once

#include <vector>

#include <boost/optional.hpp>

#include <formats/common/type.hpp>
#include <formats/serialize/to.hpp>
#include <utils/meta.hpp>

namespace formats::serialize {

// Serialize() for std::vector, std::set, std::unordered_set
template <typename T, typename Value>
std::enable_if_t<meta::is_vector<T>::value || meta::is_set<T>::value, Value>
Serialize(const T& value, To<Value>) {
  typename Value::Builder builder(formats::common::Type::kArray);
  for (const auto& item : value) {
    builder.PushBack(item);
  }
  return builder.ExtractValue();
}

// Serialize() for std::map, std::unordered_map
template <typename T, typename Value>
std::enable_if_t<meta::is_map<T>::value, Value> Serialize(const T& value,
                                                          To<Value>) {
  typename Value::Builder builder(formats::common::Type::kObject);
  for (auto it = value.begin(); it != value.end(); ++it) {
    builder[it->first] = it->second;
  }
  return builder.ExtractValue();
}

template <typename T, typename Value>
Value Serialize(const boost::optional<T>& value, To<Value>) {
  if (!value) return {};

  return typename Value::Builder(*value).ExtractValue();
}

}  // namespace formats::serialize
