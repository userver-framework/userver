#pragma once

/// @file userver/utils/statistics/json.hpp
/// @brief Statistics output in JSON format.

#include <string>
#include <unordered_map>

#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

/// Output `statistics` in JSON format:
/// @code
/// {
///   "metric-path": [
///     {
///       "value": 42,
///       "labels": {
///         "some-label": "label-value",
///         "some-other-label": "other-label-value",
///       }
///     },
///     {
///       "value": 43,
///       "labels": {
///         "another-label": "another-value"
///       }
///     },
///   ]
/// }
/// @endcode
///
/// @note `common_labels` should not match labels from `statistics`.
std::string ToJsonFormat(
    const std::unordered_map<std::string, std::string>& common_labels,
    const utils::statistics::Storage& statistics,
    const utils::statistics::StatisticsRequest& statistics_request = {});

}  // namespace utils::statistics

USERVER_NAMESPACE_END
