#include <userver/utils/statistics/solomon.hpp>

#include <array>

#include <userver/formats/json/string_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/statistics/storage.hpp>

#include <utils/statistics/solomon_limits.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {
namespace {

constexpr bool IsReservedLabelName(std::string_view name) {
  for (const auto& banned_name : impl::solomon::kReservedLabelNames) {
    if (name == banned_name) return true;
  }
  return false;
}
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
    value.Visit([this](auto x) { WriteToStream(x, builder_); });
  }

  void AddCommonLabels(
      const std::unordered_map<std::string, std::string>& common_labels) {
    if (common_labels.empty()) {
      return;
    }
    builder_.Key("commonLabels");
    WriteToStream(common_labels, builder_);
  }

 private:
  void DumpLabels(std::string_view path, utils::statistics::LabelsSpan labels) {
    formats::json::StringBuilder::ObjectGuard guard{builder_};
    builder_.Key("sensor");

    if (path.size() > impl::solomon::kMaxLabelValueLen) {
      LOG_LIMITED_WARNING()
          << "Path '" << path << "' is too long for Solomon; will be truncated";
    }
    builder_.WriteString(path.substr(0, impl::solomon::kMaxLabelValueLen));

    std::size_t written_labels = 0;
    for (const auto& label : labels) {
      const auto name = label.Name();

      const bool is_reserved = IsReservedLabelName(name);
      if (is_reserved) {
        LOG_LIMITED_ERROR() << "Label '" << name
                            << "': this name is reserved and cannot be used in "
                               "Solomon service metrics, skipping";
        UASSERT(false);
        continue;
      }

      if (written_labels >= impl::solomon::kMaxLabels) {
        LOG_LIMITED_ERROR()
            << "Label '" << label.Name()
            << "' exceeds labels limit for Solomon; will be discarded";
        UASSERT(false);
        continue;
      }

      if (name.size() > impl::solomon::kMaxLabelNameLen) {
        LOG_LIMITED_WARNING() << "Label '" << name << "': name is longer than "
                              << impl::solomon::kMaxLabelNameLen
                              << " chars allowed in Solomon; will be truncated";
      }
      builder_.Key(name.substr(0, impl::solomon::kMaxLabelNameLen));

      const auto value = label.Value();
      if (value.size() > impl::solomon::kMaxLabelValueLen) {
        LOG_LIMITED_WARNING()
            << "Label's '" << name << "': value '" << value
            << "' is longer than " << impl::solomon::kMaxLabelValueLen
            << " chars allowed in Solomon; will be truncated";
      }
      builder_.WriteString(value.substr(0, impl::solomon::kMaxLabelValueLen));

      ++written_labels;
    }
  }

  formats::json::StringBuilder& builder_;
};

}  // namespace

std::string ToSolomonFormat(
    const utils::statistics::Storage& statistics,
    const std::unordered_map<std::string, std::string>& common_labels,
    const utils::statistics::Request& request) {
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
