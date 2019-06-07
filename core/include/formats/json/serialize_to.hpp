#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <formats/json/value_builder.hpp>

namespace formats::json {

template <typename T>
std::enable_if_t<std::is_integral<T>::value && (sizeof(T) > 1),
                 formats::json::Value>
SerializeToJson(T value) {
  return formats::json::ValueBuilder(value).ExtractValue();
}

inline formats::json::Value SerializeToJson(const std::string& value) {
  return formats::json::ValueBuilder(value).ExtractValue();
}

inline formats::json::Value SerializeToJson(double value) {
  return formats::json::ValueBuilder(value).ExtractValue();
}

template <typename T>
formats::json::Value SerializeToJson(const std::vector<T>& value) {
  formats::json::ValueBuilder builder(formats::json::Type::kArray);

  for (const auto& item : value) builder.PushBack(SerializeToJson(item));

  return builder.ExtractValue();
}

template <typename T>
formats::json::Value SerializeToJson(
    const std::unordered_map<std::string, T>& value) {
  formats::json::ValueBuilder builder(formats::json::Type::kObject);

  for (const auto& it : value) builder[it.first] = SerializeToJson(it.second);

  return builder.ExtractValue();
}

}  // namespace formats::json
