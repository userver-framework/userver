#pragma once

/// @file userver/utils/statistics/metrics_storage.hpp
/// @brief @copybrief utils::statistics::MetricsStorage

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
    /// @brief Creates `MetricStorage` that considers all `MetricTag<Metric>` instances
    MetricsStorage();

    /// @brief Creates `MetricStorage` that considers only `MetricTag<Metric>` instances whose paths are listed in
    /// `allowed_metric_paths`. If `allowed_metric_paths` equals `std::nullopt`, then all `MetricTag<Metric>` instances
    /// considered
    /// @param allowed_metric_paths paths of `MetricTag<Metric>` instances that should be considered or
    /// `std::nullopt` if all `MetricTag<Metric>` instances should be considered
    MetricsStorage(const std::optional<std::vector<std::string>>& allowed_metric_paths);

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
