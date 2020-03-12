#include <logging/log.hpp>
#include <utils/statistics/metrics_storage.hpp>

#include <formats/json/serialize.hpp>
#include <utils/assert.hpp>
#include <utils/statistics/value_builder_helpers.hpp>

namespace utils::statistics {

namespace impl {

namespace {

/* MetricTag<T> may be registered only from global object ctr */
std::atomic<bool> registration_finished_{false};

std::unordered_map<std::type_index, impl::MetricInfo>& GetRegisteredMetrics() {
  static std::unordered_map<std::type_index, impl::MetricInfo> map;
  return map;
}

}  // namespace

void RegisterMetricInfo(std::type_index ti, MetricInfo&& metric_info) {
  UASSERT(!registration_finished_);

  GetRegisteredMetrics()[ti] = std::move(metric_info);
}

}  // namespace impl

MetricsStorage::MetricsStorage() : metrics_(impl::GetRegisteredMetrics()) {}

formats::json::ValueBuilder MetricsStorage::DumpMetrics() {
  impl::registration_finished_ = true;

  formats::json::ValueBuilder builder(formats::json::Type::kObject);
  for (auto& [_, metric_info] : metrics_) {
    LOG_DEBUG() << "dumping custom metric " << metric_info.path;
    auto metric = metric_info.dump_func(metric_info.data_).ExtractValue();
    if (metric.IsObject()) {
      SetSubField(builder, metric_info.path, std::move(metric));
    } else {
      builder[metric_info.path] = std::move(metric);
    }
  }
  return builder;
}

}  // namespace utils::statistics
