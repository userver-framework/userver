#pragma once

#include <userver/utils/statistics/metric_tag.hpp>

USERVER_NAMESPACE_BEGIN

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

/// @brief Storage of metrics registered with MetricTag<Metric>
/// @note The class is thread-safe. See also the note about thread-safety
/// on MetricTag<Metric>.
class MetricsStorage final {
 public:
  MetricsStorage();

  /// Get metric data by type
  template <typename Metric>
  Metric& GetMetric(const MetricTag<Metric>& tag) {
    return *std::any_cast<std::shared_ptr<Metric>&>(
        metrics_.at({typeid(Metric), tag.GetPath()}).data_);
  }

  formats::json::ValueBuilder DumpMetrics(std::string_view prefix);

  void ResetMetrics();

 private:
  impl::MetricTagMap metrics_;
};
using MetricsStoragePtr = std::shared_ptr<MetricsStorage>;

}  // namespace utils::statistics

USERVER_NAMESPACE_END
