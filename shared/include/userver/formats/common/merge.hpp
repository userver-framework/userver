#pragma once

/// @file userver/formats/common/merge.hpp
/// @brief @copybrief formats::json::MergeSchemas

#include <userver/formats/common/items.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::common {

/// @brief Add to `original` new non-object elements from `patch` (overwriting
/// the old ones, if any) and merge object elements recursively.
/// @note For the missing `patch`, it does nothing with the `original`.
/// @note Arrays are not merged.
template <typename Value>
void Merge(typename Value::Builder& original, const Value& patch) {
  if (patch.IsObject() && original.IsObject() && !original.IsEmpty()) {
    for (const auto& [elem_key, elem_value] : common::Items(patch)) {
      auto next_origin = original[elem_key];
      Merge(next_origin, elem_value);
    }
  } else if (!patch.IsMissing()) {
    original = patch;
  }
}

}  // namespace formats::common

USERVER_NAMESPACE_END
