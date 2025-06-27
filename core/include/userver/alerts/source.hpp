#pragma once

/// @file userver/alerts/source.hpp
/// @brief @copybrief alerts::Source

#include <chrono>

#include <userver/utils/statistics/metric_tag.hpp>
#include <userver/utils/statistics/metrics_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace alerts {

namespace impl {
struct SourceData {
    mutable std::atomic<bool> fired{false};
    std::atomic<std::chrono::steady_clock::time_point> stop_timepoint{{}};

    bool IsExpired() const;
};

void DumpMetric(utils::statistics::Writer& writer, const SourceData& m);
}  // namespace impl

/// @brief Alert source instance which is used to fire alerts via metrics.
class Source final {
public:
    static constexpr std::chrono::seconds kDefaultDuration{120};
    static constexpr std::chrono::hours kInfiniteDuration{24 * 365 * 10};  // In 10 years, someone should notice.

    explicit Source(const std::string& name);

    /// Fire alert for duration seconds.
    void FireAlert(utils::statistics::MetricsStorage& storage, std::chrono::seconds duration = kDefaultDuration);

    /// Stop fired alert
    void StopAlertNow(utils::statistics::MetricsStorage& storage);

private:
    utils::statistics::MetricTag<impl::SourceData> tag_;
};

}  // namespace alerts

USERVER_NAMESPACE_END
