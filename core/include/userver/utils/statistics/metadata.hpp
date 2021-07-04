#pragma once

/// @file userver/utils/statistics/metadata.hpp
/// @brief Definitions for metrics metadata

#include <string>

#include <userver/formats/json/value_builder.hpp>

namespace utils {
namespace statistics {

/// @brief Marks statistics node to be excluded from Solomon sensor name.
/// @warning Cannot be applied to leaf nodes.
void SolomonSkip(formats::json::ValueBuilder& stats_node);

/// @brief Marks statistics node to be excluded from Solomon sensor name.
/// @warning Cannot be applied to leaf nodes.
void SolomonSkip(formats::json::ValueBuilder&& stats_node);

/// @brief Replaces a node in Solomon sensor name.
/// @warning Cannot be applied to leaf nodes.
void SolomonRename(formats::json::ValueBuilder& stats_node,
                   const std::string& new_name);

/// @brief Replaces a node in Solomon sensor name.
/// @warning Cannot be applied to leaf nodes.
void SolomonRename(formats::json::ValueBuilder&& stats_node,
                   const std::string& new_name);

/// Moves statistic node name to label value for Solomon sensor.
/// @warning Cannot be applied to leaf nodes.
void SolomonLabelValue(formats::json::ValueBuilder& stats_node,
                       const std::string& label_name);

/// Moves statistic node name to label value for Solomon sensor.
/// @warning Cannot be applied to leaf nodes.
void SolomonLabelValue(formats::json::ValueBuilder&& stats_node,
                       const std::string& label_name);

/// Moves statistics node children names to label values for Solomon sensors.
void SolomonChildrenAreLabelValues(formats::json::ValueBuilder& stats_node,
                                   const std::string& label_name);

/// Moves statistics node children names to label values for Solomon sensors.
void SolomonChildrenAreLabelValues(formats::json::ValueBuilder&& stats_node,
                                   const std::string& label_name);

}  // namespace statistics
}  // namespace utils
