#pragma once

/// @file userver/utils/statistics/json.hpp
/// @brief Statistics output in JSON format.

#include <string>

#include <userver/utils/statistics/request.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/utils/statistics/writer.hpp>

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
std::string ToJsonFormat(const Storage& statistics, const Request& statistics_request = {});

std::string ToJsonFormat(WriterFuncRef writer, const Request& statistics_request = {});

}  // namespace utils::statistics

USERVER_NAMESPACE_END
