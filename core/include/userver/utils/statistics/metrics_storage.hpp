#pragma once

#include <userver/utils/statistics/metric_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

/// @brief Storage of metrics registered with MetricTag<Metric>
/// @note The class is thread-safe. See also the note about thread-safety
/// on MetricTag<Metric>.
class MetricsStorage final {
 public:
  MetricsStorage();

  /// Get metric data by type
  template <typename Metric>
  Metric& GetMetric(const MetricTag<Metric>& tag) {
    return impl::GetMetric<Metric>(metrics_, tag.key_);
  }

  formats::json::ValueBuilder DumpMetrics(std::string_view prefix);

  void ResetMetrics();

 private:
  impl::MetricMap metrics_;
};

using MetricsStoragePtr = std::shared_ptr<MetricsStorage>;

}  // namespace utils::statistics

USERVER_NAMESPACE_END
