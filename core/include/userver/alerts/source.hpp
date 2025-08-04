#pragma once

/// @file userver/alerts/source.hpp
/// @brief @copybrief alerts::Source

#include <chrono>
#include <string_view>

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

/// @brief Alert source instance which is used to fire alerts via metrics for a specified amount of time.
///
/// To declare an alert:
/// @snippet core/src/logging/component.cpp  alert_declaration
///
/// Tu fire or to stop an alert:
/// @snippet core/src/logging/component.cpp  alert_usage
///
/// For non alert-metrics consider using utils::statistics::MetricTag.
class Source final {
public:
    static constexpr std::chrono::seconds kDefaultDuration{120};
    static constexpr std::chrono::hours kInfiniteDuration{24 * 365 * 10};  // In 10 years, someone should notice.

    /// Constructs an alert source instance that will be reported as non-zero "alerts." + std::string{name} metric
    /// in case of error
    explicit Source(std::string_view name);

    /// Fire alert for duration seconds.
    void FireAlert(utils::statistics::MetricsStorage& storage, std::chrono::seconds duration = kDefaultDuration) const;

    /// Stop fired alert
    void StopAlertNow(utils::statistics::MetricsStorage& storage) const;

private:
    utils::statistics::MetricTag<impl::SourceData> tag_;
};

}  // namespace alerts

USERVER_NAMESPACE_END
