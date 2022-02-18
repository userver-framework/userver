#include <userver/utils/statistics/metrics_storage.hpp>

#include <functional>
#include <typeindex>

#include <fmt/format.h>

#include <boost/functional/hash.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/text.hpp>
#include <utils/statistics/value_builder_helpers.hpp>

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

formats::json::ValueBuilder MetricsStorage::DumpMetrics(
    std::string_view prefix) {
  formats::json::ValueBuilder builder(formats::json::Type::kObject);
  for (auto& [key, value] : metrics_) {
    if (!utils::text::StartsWith(key.path, prefix) &&
        !utils::text::StartsWith(prefix, key.path)) {
      LOG_DEBUG() << "skipping custom metric " << key.path;
      continue;
    } else {
      LOG_DEBUG() << "dumping custom metric " << key.path;
    }
    auto metric = value->Dump().ExtractValue();
    if (metric.IsObject()) {
      SetSubField(builder, SplitPath(key.path), std::move(metric));
    } else {
      builder[key.path] = std::move(metric);
    }
  }
  return builder;
}

void MetricsStorage::ResetMetrics() {
  LOG_DEBUG() << "resetting custom metric";

  for (auto& [_, value] : metrics_) {
    value->Reset();
  }
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
