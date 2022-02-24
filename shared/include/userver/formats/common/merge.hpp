#pragma once

/// @file userver/formats/common/merge.hpp
/// @brief @copybrief formats::json::Merge

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::common {

/// @brief Add to `original` new non-object elements from `patch` (overwriting
/// the old ones, if any) and merge object elements recursively.
/// @note For the missing `patch`, it does nothing with the `original`.
/// @note Arrays are not merged.
void Merge(formats::json::ValueBuilder& original,
           const formats::json::Value& patch);

}  // namespace formats::common

USERVER_NAMESPACE_END
