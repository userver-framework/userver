#include <userver/alerts/source.hpp>

USERVER_NAMESPACE_BEGIN

namespace alerts {

namespace impl {
bool SourceData::IsExpired() const { return stop_timepoint.load() < std::chrono::steady_clock::now(); }

void DumpMetric(utils::statistics::Writer& writer, const SourceData& m) {
    if (m.IsExpired()) m.fired = false;
    writer = m.fired;
}
}  // namespace impl

Source::Source(const std::string& name) : tag_("alerts." + name) {}

void Source::FireAlert(utils::statistics::MetricsStorage& storage, std::chrono::seconds duration) {
    auto& metric = storage.GetMetric(tag_);
    metric.stop_timepoint = std::chrono::steady_clock::now() + duration;
    metric.fired = true;
}

void Source::StopAlertNow(utils::statistics::MetricsStorage& storage) {
    auto& metric = storage.GetMetric(tag_);
    metric.fired = false;
}

}  // namespace alerts

USERVER_NAMESPACE_END
