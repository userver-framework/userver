#pragma once

#include <vector>

#include <formats/json/value_builder.hpp>
#include <formats/parse/common_containers.hpp>
#include <formats/serialize/to.hpp>
#include <utils/meta.hpp>

namespace formats::serialize {

// Serialize() for std::vector, std::set, std::unordered_set
template <typename T>
std::enable_if_t<meta::is_vector<T>::value || meta::is_set<T>::value,
                 ::formats::json::Value>
Serialize(const T& value, To<::formats::json::Value>) {
  ::formats::json::ValueBuilder builder(json::Type::kArray);
  for (const auto& item : value) {
    builder.PushBack(item);
  }
  return builder.ExtractValue();
}

// Serialize() for std::map, std::unordered_map
template <typename T>
std::enable_if_t<meta::is_map<T>::value, ::formats::json::Value> Serialize(
    const T& value, To<::formats::json::Value>) {
  ::formats::json::ValueBuilder builder(json::Type::kObject);
  for (auto it = value.begin(); it != value.end(); ++it) {
    builder[it->first] = it->second;
  }
  return builder.ExtractValue();
}

template <typename T>
json::Value Serialize(const boost::optional<T>& value, To<json::Value>) {
  if (!value) return {};

  return json::ValueBuilder(*value).ExtractValue();
}

}  // namespace formats::serialize
