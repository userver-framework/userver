#include <utils/statistics/metrics_storage.hpp>

#include <functional>
#include <typeindex>

#include <boost/functional/hash.hpp>
#include <formats/json/serialize.hpp>
#include <logging/log.hpp>
#include <utils/assert.hpp>
#include <utils/statistics/value_builder_helpers.hpp>

namespace utils::statistics {

namespace impl {

namespace {

/* MetricTag<T> may be registered only from global object ctr */
std::atomic<bool> registration_finished_{false};

MetricTagMap& GetRegisteredMetrics() {
  static MetricTagMap map;
  return map;
}

}  // namespace

size_t MetricKeyHash::operator()(const MetricKey& key) const noexcept {
  auto seed = std::hash<std::type_index>()(key.idx);
  boost::hash_combine(seed, std::hash<std::string>()(key.path));
  return seed;
}

void RegisterMetricInfo(std::type_index ti, MetricInfo&& metric_info) {
  UASSERT(!registration_finished_);

  auto path = metric_info.path;
  auto [_, ok] = GetRegisteredMetrics().emplace(MetricKey{ti, path},
                                                std::move(metric_info));
  UASSERT_MSG(ok, "duplicate MetricTag with path '" + path + "'");
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

void MetricsStorage::ResetMetrics() {
  impl::registration_finished_ = true;

  LOG_DEBUG() << "reseting custom metric";

  for (auto& [_, metric_info] : metrics_) {
    metric_info.reset_func(metric_info.data_);
  }
}

}  // namespace utils::statistics
