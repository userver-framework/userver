#include <userver/utils/statistics/solomon.hpp>

#include <array>

#include <userver/formats/json/string_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {
namespace {

// These labels are always applied and cannot be set by user
constexpr std::array<std::string_view, 6> kReservedLabelNames = {
    "project", "cluster", "service", "host", "group", "sensor"};

constexpr bool IsReservedLabelName(std::string_view name) {
  for (const auto& banned_name : kReservedLabelNames) {
    if (name == banned_name) return true;
  }
  return false;
}

// https://solomon.yandex-team.ru/docs/concepts/data-model#limits
// "application" is in commonLabels and can be overridden (nginx system metrics)
constexpr size_t kMaxLabels = 16 - kReservedLabelNames.size() - 1;
constexpr size_t kMaxLabelNameLen = 31;
constexpr size_t kMaxLabelValueLen = 200;

class SolomonJsonBuilder final : public utils::statistics::BaseFormatBuilder {
 public:
  explicit SolomonJsonBuilder(formats::json::StringBuilder& builder)
      : builder_{builder} {}

  void HandleMetric(std::string_view path, utils::statistics::LabelsSpan labels,
                    const MetricValue& value) override {
    formats::json::StringBuilder::ObjectGuard guard{builder_};
    builder_.Key("labels");
    DumpLabels(path, labels);
    builder_.Key("value");
    if (const auto* int_value = std::get_if<int64_t>(&value)) {
      builder_.WriteInt64(*int_value);
    } else {
      builder_.WriteDouble(std::get<double>(value));
    }
  }

  void AddCommonLabels(
      const std::unordered_map<std::string, std::string>& common_labels) {
    if (common_labels.empty()) {
      return;
    }
    formats::json::StringBuilder common_labels_builder;
    {
      formats::json::StringBuilder::ObjectGuard guard(common_labels_builder);
      for (const auto& [label, value] : common_labels) {
        common_labels_builder.Key(label);
        common_labels_builder.WriteString(value);
      }
    }
    builder_.Key("commonLabels");
    builder_.WriteRawString(common_labels_builder.GetString());
  }

 private:
  void DumpLabels(std::string_view path, utils::statistics::LabelsSpan labels) {
    formats::json::StringBuilder::ObjectGuard guard{builder_};
    builder_.Key("sensor");

    if (path.size() > kMaxLabelValueLen) {
      LOG_LIMITED_WARNING()
          << "Path '" << path << "' is too long for Solomon; will be truncated";
    }
    builder_.WriteString(path.substr(0, kMaxLabelValueLen));

    std::size_t written_labels = 0;
    for (const auto& label : labels) {
      const auto& name = label.Name();

      const bool is_reserved = IsReservedLabelName(name);
      if (is_reserved) {
        LOG_LIMITED_ERROR() << "Label '" << name
                            << "': this name is reserved and cannot be used in "
                               "Solomon service metrics, skipping";
        UASSERT(false);
        continue;
      }

      if (written_labels >= kMaxLabels) {
        LOG_LIMITED_ERROR()
            << "Label '" << label.Name()
            << "' exceeds labels limit for Solomon; will be discarded";
        UASSERT(false);
        continue;
      }

      if (name.size() > kMaxLabelNameLen) {
        LOG_LIMITED_WARNING() << "Label '" << name << "': name is longer than "
                              << kMaxLabelNameLen
                              << " chars allowed in Solomon; will be truncated";
      }
      builder_.Key(std::string_view{name}.substr(0, kMaxLabelNameLen));

      const auto& value = label.Value();
      if (value.size() > kMaxLabelValueLen) {
        LOG_LIMITED_WARNING() << "Label's '" << name << "': value '" << value
                              << "' is longer than " << kMaxLabelValueLen
                              << " chars allowed in Solomon; will be truncated";
      }
      builder_.WriteString(
          std::string_view{value}.substr(0, kMaxLabelValueLen));

      ++written_labels;
    }
  }

  formats::json::StringBuilder& builder_;
};

}  // namespace

std::string ToSolomonFormat(
    const utils::statistics::Storage& statistics,
    const std::unordered_map<std::string, std::string>& common_labels,
    const utils::statistics::StatisticsRequest& request) {
  formats::json::StringBuilder builder;
  SolomonJsonBuilder solomon_json_builder(builder);
  {
    formats::json::StringBuilder::ObjectGuard object_guard(builder);
    solomon_json_builder.AddCommonLabels(common_labels);

    builder.Key("metrics");
    formats::json::StringBuilder::ArrayGuard array_guard(builder);
    statistics.VisitMetrics(solomon_json_builder, request);
  }
  return builder.GetString();
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
