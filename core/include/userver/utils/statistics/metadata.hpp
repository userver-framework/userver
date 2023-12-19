#pragma once

/// @file userver/utils/statistics/metadata.hpp
/// @deprecated Use utils::statistics::Writer instead.

#include <string>

#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

/// @brief Marks statistics node to be excluded from Solomon sensor name.
/// @deprecated Use utils::statistics::Writer instead.
/// @warning Cannot be applied to leaf nodes.
void SolomonSkip(formats::json::ValueBuilder& stats_node);

/// @brief Marks statistics node to be excluded from Solomon sensor name.
/// @deprecated Use utils::statistics::Writer instead.
/// @warning Cannot be applied to leaf nodes.
void SolomonSkip(formats::json::ValueBuilder&& stats_node);

/// @brief Replaces a node in Solomon sensor name.
/// @deprecated Use utils::statistics::Writer instead.
/// @warning Cannot be applied to leaf nodes.
void SolomonRename(formats::json::ValueBuilder& stats_node,
                   const std::string& new_name);

/// @brief Replaces a node in Solomon sensor name.
/// @deprecated Use utils::statistics::Writer instead.
/// @warning Cannot be applied to leaf nodes.
void SolomonRename(formats::json::ValueBuilder&& stats_node,
                   const std::string& new_name);

/// Moves statistic node name to label value for Solomon sensor.
/// @deprecated Use utils::statistics::Writer instead.
/// @warning Cannot be applied to leaf nodes.
void SolomonLabelValue(formats::json::ValueBuilder& stats_node,
                       const std::string& label_name);

/// Moves statistic node name to label value for Solomon sensor.
/// @deprecated Use utils::statistics::Writer instead.
/// @warning Cannot be applied to leaf nodes.
void SolomonLabelValue(formats::json::ValueBuilder&& stats_node,
                       const std::string& label_name);

/// Moves statistics node children names to label values for Solomon sensors.
/// @deprecated Use utils::statistics::Writer instead.
void SolomonChildrenAreLabelValues(formats::json::ValueBuilder& stats_node,
                                   const std::string& label_name);

/// Moves statistics node children names to label values for Solomon sensors.
/// @deprecated Use utils::statistics::Writer instead.
void SolomonChildrenAreLabelValues(formats::json::ValueBuilder&& stats_node,
                                   const std::string& label_name);

}  // namespace utils::statistics

USERVER_NAMESPACE_END
