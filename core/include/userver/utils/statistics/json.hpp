#pragma once

/// @file userver/utils/statistics/json.hpp
/// @brief Statistics output in JSON format.

#include <string>

#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

/// Output `statistics` in JSON format:
/// @code
/// {
///   "metric-path": [
///     {
///       "type": "GAUGE",
///       "value": 42,
///       "labels": {
///         "some-label": "label-value",
///         "some-other-label": "other-label-value",
///       }
///     },
///     {
///       "type": "RATE",
///       "value": 43,
///       "labels": {
///         "another-label": "another-value"
///       }
///     },
///   ]
/// }
/// @endcode
std::string ToJsonFormat(
    const utils::statistics::Storage& statistics,
    const utils::statistics::Request& statistics_request = {});

}  // namespace utils::statistics

USERVER_NAMESPACE_END
