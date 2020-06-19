#pragma once

#include <utils/statistics/metric_tag.hpp>

namespace utils::statistics {

/// Storage of metrics registered with MetricTag<Metric>
class MetricsStorage final {
 public:
  MetricsStorage();

  /// Get metric data by type
  template <typename Metric>
  Metric& GetMetric(const MetricTag<Metric>&) {
    return *std::any_cast<std::shared_ptr<Metric>&>(
        metrics_.at(typeid(Metric)).data_);
  }

  formats::json::ValueBuilder DumpMetrics();

 private:
  std::unordered_map<std::type_index, impl::MetricInfo> metrics_;
};
using MetricsStoragePtr = std::shared_ptr<MetricsStorage>;

}  // namespace utils::statistics
