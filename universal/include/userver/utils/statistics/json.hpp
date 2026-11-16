#pragma once

/// @file userver/utils/statistics/json.hpp
/// @brief Statistics output in JSON format.
/// @ingroup userver_universal

#include <string>

#include <userver/utils/statistics/request.hpp>
#include <userver/utils/statistics/visit.hpp>

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
///
/// @note An integer `GAUGE` and a floating-point `GAUGE` are both written as a JSON number
/// of the same type. In practice they cannot be told apart.
/// @note A histogram writes only @ref utils::statistics::HistogramView::GetTotalSum into `sum`.
/// Sums of individual buckets are not written.
std::string ToJsonFormat(WriterFuncRef writer, const Request& statistics_request = {});

/// @overload
std::string ToJsonFormat(const HasWriterSupport auto& metric, const Request& statistics_request = {}) {
    return ToJsonFormat([&metric](Writer& writer) { writer = metric; }, statistics_request);
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
