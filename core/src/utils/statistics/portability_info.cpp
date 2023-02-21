#include <userver/utils/statistics/portability_info.hpp>

#include <algorithm>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <fmt/format.h>

#include <userver/formats/json.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/statistics/storage.hpp>

#include <utils/statistics/solomon_limits.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

namespace {

class PortabilityInfoCollector final
    : public utils::statistics::BaseFormatBuilder {
 public:
  void HandleMetric(std::string_view path_view,
                    utils::statistics::LabelsSpan labels,
                    const MetricValue& value) override {
    const std::string path{path_view};

    value.Visit([&, this](auto f) {
      if constexpr (std::is_same_v<decltype(f), double>) {
        if (std::isinf(f)) {
          ReportError(info_[WarningCode::kInf], path, labels,
                      "Value is +/-INF");
        }

        if (std::isnan(f)) {
          ReportError(info_[WarningCode::kNan], path, labels,
                      "Value is +/-NAN");
        }
      }
    });

    static constexpr std::size_t kMaxLabelsPortableValue =
        impl::solomon::kMaxLabels;
    if (labels.size() > kMaxLabelsPortableValue) {
      ReportError(
          info_[WarningCode::kLabelsCount], path, labels,
          fmt::format(
              "To many labels: {} while recommended value is not more than {}",
              labels.size(), kMaxLabelsPortableValue));
    }

    static constexpr std::size_t kMaxLabelNameLen =
        impl::solomon::kMaxLabelNameLen;
    static constexpr std::size_t kMaxLabelValueLen =
        impl::solomon::kMaxLabelValueLen;

    std::vector<std::string> labels_store;
    labels_store.reserve(labels.size());
    for (const auto& label : labels) {
      const auto it = kReservedLabelNames.find(label.Name());
      if (it != kReservedLabelNames.end()) {
        ReportError(info_[it->second], path, labels,
                    fmt::format("Reserved label used: {}", it->first));
      }

      if (label.Name().size() > kMaxLabelNameLen) {
        ReportError(
            info_[WarningCode::kLabelNameLength], path, labels,
            fmt::format("Label '{}' exceeeds recommended label name length {}",
                        label.Name(), kMaxLabelNameLen));
      }

      if (label.Value().size() > kMaxLabelValueLen) {
        ReportError(
            info_[WarningCode::kLabelValueLength], path, labels,
            fmt::format("Value of label '{}' exceeeds recommended length {}",
                        label.Name(), kMaxLabelValueLen));
      }

      labels_store.emplace_back(label.Name());
    }
    std::sort(labels_store.begin(), labels_store.end());

    static constexpr size_t kMaxPathLen = impl::solomon::kMaxLabelValueLen;
    if (path.size() > kMaxPathLen) {
      ReportError(
          info_[WarningCode::kPathLength], path, labels,
          fmt::format("Path exceeeds recommended length {}", kMaxPathLen));
    }

    auto it = path_labels_.find(path);
    if (it == path_labels_.end()) {
      path_labels_[path] = std::move(labels_store);
    } else if (it->second != labels_store) {
      ReportError(
          info_[WarningCode::kLabelNameMismatch], path, labels,
          fmt::format("Same path '{}' has metrics with different labels set: "
                      "'{}' vs '{}'",
                      path, fmt::join(it->second, ", "),
                      fmt::join(labels_store, ", ")));
    }
  }

  PortabilityWarnings Extract() && { return std::move(info_); }

 private:
  static void ReportError(std::vector<Warning>& w, const std::string& path,
                          utils::statistics::LabelsSpan labels,
                          std::string_view error_msg) {
    Warning entry;

    entry.labels.reserve(labels.size());
    for (const auto& l : labels) {
      entry.labels.emplace_back(l);
    }

    entry.path = path;
    entry.error_message = error_msg;

    w.push_back(std::move(entry));
  }

  inline static const std::unordered_map<std::string_view, WarningCode>
      kReservedLabelNames = {
          {"application", WarningCode::kReservedLabelApplication},
          {"project", WarningCode::kReservedLabelProject},
          {"cluster", WarningCode::kReservedLabelCluster},
          {"service", WarningCode::kReservedLabelService},
          {"host", WarningCode::kReservedLabelHost},
          {"group", WarningCode::kReservedLabelGroup},
          {"sensor", WarningCode::kReservedLabelSensor},
      };

  PortabilityWarnings info_;

  std::unordered_map<std::string, std::vector<std::string>> path_labels_;
};

}  // namespace

formats::json::Value Serialize(const std::vector<Label>& labels,
                               formats::serialize::To<formats::json::Value>) {
  formats::json::ValueBuilder builder{formats::common::Type::kObject};
  for (const auto& l : labels) {
    builder[l.Name()] = l.Value();
  }

  return builder.ExtractValue();
}

formats::json::Value Serialize(const Warning& entry,
                               formats::serialize::To<formats::json::Value>) {
  formats::json::ValueBuilder builder;

  builder["error_message"] = entry.error_message;
  builder["path"] = entry.path;
  builder["labels"] = entry.labels;

  return builder.ExtractValue();
}

std::string_view ToString(WarningCode code) {
  switch (code) {
    case WarningCode::kInf:
      return "inf";
    case WarningCode::kNan:
      return "nan";
    case WarningCode::kLabelsCount:
      return "labels_count";

    case WarningCode::kReservedLabelApplication:
      return "label_reserved_application";
    case WarningCode::kReservedLabelCluster:
      return "label_reserved_cluster";
    case WarningCode::kReservedLabelGroup:
      return "label_reserved_group";
    case WarningCode::kReservedLabelHost:
      return "label_reserved_host";
    case WarningCode::kReservedLabelProject:
      return "label_reserved_project";
    case WarningCode::kReservedLabelSensor:
      return "label_reserved_sensor";
    case WarningCode::kReservedLabelService:
      return "label_reserved_service";

    case WarningCode::kLabelNameLength:
      return "label_name_length";
    case WarningCode::kLabelValueLength:
      return "label_value_length";
    case WarningCode::kPathLength:
      return "path_length";
    case WarningCode::kLabelNameMismatch:
      return "label_name_mismatch";
  }

  UINVARIANT(false, "Unhandled WarningCode value " +
                        std::to_string(static_cast<int>(code)));
}

formats::json::Value Serialize(const PortabilityWarnings& info,
                               formats::serialize::To<formats::json::Value>) {
  formats::json::ValueBuilder builder{formats::common::Type::kObject};

  for (const auto& [code, warnings] : info) {
    builder[std::string{ToString(code)}] = warnings;
  }

  return builder.ExtractValue();
}

PortabilityWarnings GetPortabilityWarnings(
    const utils::statistics::Storage& statistics,
    const utils::statistics::Request& request) {
  PortabilityInfoCollector builder{};
  statistics.VisitMetrics(builder, request);
  return std::move(builder).Extract();
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
