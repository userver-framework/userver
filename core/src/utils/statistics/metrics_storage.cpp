#include <userver/utils/statistics/metrics_storage.hpp>

#include <functional>
#include <typeindex>

#include <fmt/format.h>

#include <boost/functional/hash.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

namespace impl {

namespace {

// MetricTag<T> may be registered only from global object ctr
std::atomic<bool> registration_finished_{false};

using MetricMetadataMap =
    std::unordered_map<MetricKey, MetricFactory, MetricKeyHash>;

MetricMetadataMap& GetRegisteredMetrics() {
  static MetricMetadataMap map;
  return map;
}

MetricMap InstantiateMetrics() {
  impl::registration_finished_ = true;
  const auto& registered_metrics = GetRegisteredMetrics();

  MetricMap metrics;
  for (const auto& [key, factory] : registered_metrics) {
    metrics.emplace(key, factory());
  }
  return metrics;
}

}  // namespace

std::size_t MetricKeyHash::operator()(const MetricKey& key) const noexcept {
  auto seed = std::hash<std::type_index>()(key.idx);
  boost::hash_combine(seed, std::hash<std::string>()(key.path));
  return seed;
}

MetricWrapperBase::~MetricWrapperBase() = default;

void RegisterMetricInfo(const MetricKey& key, MetricFactory factory) {
  UASSERT(!registration_finished_);
  UASSERT(factory);
  auto [_, ok] = GetRegisteredMetrics().emplace(key, factory);
  UASSERT_MSG(ok, fmt::format("duplicate MetricTag with path '{}'", key.path));
}

}  // namespace impl

MetricsStorage::MetricsStorage() : metrics_(impl::InstantiateMetrics()) {}

std::vector<Entry> MetricsStorage::RegisterIn(Storage& statistics_storage) {
  std::vector<Entry> holders;
  holders.reserve(metrics_.size());

  for (auto& [key, metric_ptr] : metrics_) {
    auto& metric = *metric_ptr;
    holders.push_back(statistics_storage.RegisterExtender(
        key.path, [&metric](const auto& /*prefix*/) { return metric.Dump(); }));
  }

  return holders;
}

void MetricsStorage::ResetMetrics() {
  LOG_DEBUG() << "resetting custom metric";

  for (auto& [_, value] : metrics_) {
    value->Reset();
  }
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
