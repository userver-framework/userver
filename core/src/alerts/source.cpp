#include <userver/alerts/source.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace alerts {

namespace impl {
bool SourceData::IsExpired() const { return stop_timepoint.load() < std::chrono::steady_clock::now(); }

void DumpMetric(utils::statistics::Writer& writer, const SourceData& m) {
    if (m.IsExpired()) m.fired = false;
    writer = m.fired;
}
}  // namespace impl

Source::Source(std::string_view name) : tag_(fmt::format("alerts.{}", name)) {}

void Source::FireAlert(utils::statistics::MetricsStorage& storage, std::chrono::seconds duration) const {
    auto& metric = storage.GetMetric(tag_);
    metric.stop_timepoint = std::chrono::steady_clock::now() + duration;
    metric.fired = true;
}

void Source::StopAlertNow(utils::statistics::MetricsStorage& storage) const {
    auto& metric = storage.GetMetric(tag_);
    metric.fired = false;
}

}  // namespace alerts

USERVER_NAMESPACE_END
