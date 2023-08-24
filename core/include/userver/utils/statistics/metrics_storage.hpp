#pragma once

#include <vector>

#include <userver/utils/statistics/fwd.hpp>
#include <userver/utils/statistics/metric_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

/// @brief Storage of metrics registered with MetricTag<Metric>
/// @note The class is thread-safe. See also the note about thread-safety
/// on MetricTag<Metric>.
class MetricsStorage final {
 public:
  MetricsStorage();

  [[nodiscard]] std::vector<Entry> RegisterIn(Storage& statistics_storage);

  /// Get metric data by type
  template <typename Metric>
  Metric& GetMetric(const MetricTag<Metric>& tag) {
    return impl::GetMetric<Metric>(metrics_, tag.key_);
  }

  void ResetMetrics();

 private:
  impl::MetricMap metrics_;
};

using MetricsStoragePtr = std::shared_ptr<MetricsStorage>;

}  // namespace utils::statistics

USERVER_NAMESPACE_END
