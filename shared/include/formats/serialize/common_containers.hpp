#pragma once

/// @file formats/serialize/common_containers.hpp
/// @brief Serializers for standard containers and optional

#include <array>
#include <optional>
#include <type_traits>
#include <vector>

#include <formats/common/type.hpp>
#include <formats/serialize/to.hpp>
#include <utils/meta.hpp>

/// Common serializers
namespace formats::serialize {

/// Common containers serialization (vector/set)
template <typename T, typename Value>
std::enable_if_t<meta::kIsRange<T> && !meta::kIsMap<T>, Value> Serialize(
    const T& value, To<Value>) {
  typename Value::Builder builder(formats::common::Type::kArray);
  for (const auto& item : value) {
    // explicit cast for vector<bool> shenanigans
    builder.PushBack(static_cast<const meta::RangeValueType<T>&>(item));
  }
  return builder.ExtractValue();
}

/// Mappings serialization
template <typename T, typename Value>
std::enable_if_t<meta::kIsMap<T>, Value> Serialize(const T& value, To<Value>) {
  typename Value::Builder builder(formats::common::Type::kObject);
  for (const auto& [key, value] : value) {
    builder[key] = value;
  }
  return builder.ExtractValue();
}

/// std::optional serialization
template <typename T, typename Value>
Value Serialize(const std::optional<T>& value, To<Value>) {
  if (!value) return {};

  return typename Value::Builder(*value).ExtractValue();
}

}  // namespace formats::serialize
