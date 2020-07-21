#pragma once

#include <utils/statistics/metric_tag.hpp>

namespace utils::statistics {

namespace impl {

struct MetricKey {
  std::type_index idx;
  std::string path;

  bool operator==(const MetricKey& other) const noexcept {
    return idx == other.idx && path == other.path;
  }
};

struct MetricKeyHash {
  size_t operator()(const MetricKey& key) const noexcept;
};

using MetricTagMap =
    std::unordered_map<MetricKey, impl::MetricInfo, MetricKeyHash>;
}  // namespace impl

/// Storage of metrics registered with MetricTag<Metric>
class MetricsStorage final {
 public:
  MetricsStorage();

  /// Get metric data by type
  template <typename Metric>
  Metric& GetMetric(const MetricTag<Metric>& tag) {
    return *std::any_cast<std::shared_ptr<Metric>&>(
        metrics_.at({typeid(Metric), tag.GetPath()}).data_);
  }

  formats::json::ValueBuilder DumpMetrics();

 private:
  impl::MetricTagMap metrics_;
};
using MetricsStoragePtr = std::shared_ptr<MetricsStorage>;

}  // namespace utils::statistics
