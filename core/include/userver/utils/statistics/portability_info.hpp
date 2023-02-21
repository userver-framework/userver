#pragma once

/// @file userver/utils/statistics/portability_info.hpp
/// @brief Portability reports.

#include <string>

#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

formats::json::Value Serialize(const std::vector<Label>& labels,
                               formats::serialize::To<formats::json::Value>);

struct Warning {
  std::string error_message;
  std::string path;
  std::vector<Label> labels;
};

formats::json::Value Serialize(const Warning& entry,
                               formats::serialize::To<formats::json::Value>);

enum class WarningCode {
  kInf,
  kNan,
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

using PortabilityWarnings =
    std::unordered_map<WarningCode, std::vector<Warning>>;

/// JSON serilization for the PortabilityInfo in the following format:
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
formats::json::Value Serialize(const PortabilityWarnings& info,
                               formats::serialize::To<formats::json::Value>);

/// Output portability info for `statistics`.
/// @see @ref md_en_userver_functional_testing
PortabilityWarnings GetPortabilityWarnings(
    const utils::statistics::Storage& statistics,
    const utils::statistics::Request& request);

}  // namespace utils::statistics

USERVER_NAMESPACE_END
