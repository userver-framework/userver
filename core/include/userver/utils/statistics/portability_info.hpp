#pragma once

/// @file userver/utils/statistics/portability_info.hpp
/// @brief Portability reports.

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <userver/formats/json/value.hpp>
#include <userver/utils/statistics/labels.hpp>
#include <userver/utils/statistics/request.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

formats::json::Value Serialize(const std::vector<Label>& labels, formats::serialize::To<formats::json::Value>);

struct Warning {
    std::string error_message;
    std::string path;
    std::vector<Label> labels;
};

formats::json::Value Serialize(const Warning& entry, formats::serialize::To<formats::json::Value>);

enum class WarningCode {
    kInf,
    kNan,
    kHistogramBucketsCount,

    kLabelsCount,

    kReservedLabelApplication,
    kReservedLabelCluster,
    kReservedLabelGroup,
    kReservedLabelHost,
    kReservedLabelProject,
    kReservedLabelSensor,
    kReservedLabelService,

    kLabelNameLength,
    kLabelValueLength,
    kPathLength,
    kLabelNameMismatch,
};

std::string_view ToString(WarningCode code);

using PortabilityWarnings = std::unordered_map<WarningCode, std::vector<Warning>>;

/// JSON serialization for the PortabilityInfo in the following format:
/// @code
/// {
///   "warning_code": [
///     {
///       "error_message": "Human readable message",
///       "path": "foo-bar",
///       "labels": {
///         "some-label": "label-value",
///         "some-other-label": "other-label-value",
///       }
///     }
///   ],
///   "another_error_id": [
///     {
///       "path": "foo.bar",
///       "labels": {
///         "another-label": "another-value"
///       }
///     },
///   ]
/// }
/// @endcode
formats::json::Value Serialize(const PortabilityWarnings& info, formats::serialize::To<formats::json::Value>);

/// Output portability info for `statistics`.
/// @see @ref scripts/docs/en/userver/functional_testing.md
PortabilityWarnings GetPortabilityWarnings(const Storage& statistics, const Request& request);

PortabilityWarnings GetPortabilityWarnings(WriterFuncRef writer, const Request& request);

}  // namespace utils::statistics

USERVER_NAMESPACE_END
