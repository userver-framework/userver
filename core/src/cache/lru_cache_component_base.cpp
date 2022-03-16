#include <userver/cache/lru_cache_component_base.hpp>

#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/taxi_config/storage/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/statistics/metadata.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache::impl {

namespace {

constexpr const char* kStatisticsNameHits = "hits";
constexpr const char* kStatisticsNameMisses = "misses";
constexpr const char* kStatisticsNameStale = "stale";
constexpr const char* kStatisticsNameBackground = "background-updates";
constexpr const char* kStatisticsNameHitRatio = "hit_ratio";
constexpr const char* kStatisticsNameCurrentDocumentsCount =
    "current-documents-count";

}  // namespace

formats::json::Value GetCacheStatisticsAsJson(
    const ExpirableLruCacheStatistics& stats, std::size_t size) {
  formats::json::ValueBuilder builder;
  utils::statistics::SolomonLabelValue(builder, "cache_name");

  builder[kStatisticsNameCurrentDocumentsCount] = size;
  builder[kStatisticsNameHits] = stats.total.hits.load();
  builder[kStatisticsNameMisses] = stats.total.misses.load();
  builder[kStatisticsNameStale] = stats.total.stale.load();
  builder[kStatisticsNameBackground] = stats.total.background_updates.load();

  auto s1min = stats.recent.GetStatsForPeriod();
  double s1min_hits = s1min.hits.load();
  auto s1min_total = s1min.hits.load() + s1min.misses.load();
  builder[kStatisticsNameHitRatio]["1min"] =
      s1min_hits / static_cast<double>(s1min_total ? s1min_total : 1);
  return builder.ExtractValue();
}

testsuite::ComponentControl& FindComponentControl(
    const components::ComponentContext& context) {
  return context.FindComponent<components::TestsuiteSupport>()
      .GetComponentControl();
}

utils::statistics::Storage& FindStatisticsStorage(
    const components::ComponentContext& context) {
  return context.FindComponent<components::StatisticsStorage>().GetStorage();
}

dynamic_config::Source FindDynamicConfigSource(
    const components::ComponentContext& context) {
  return context.FindComponent<components::DynamicConfig>().GetSource();
}

}  // namespace cache::impl

USERVER_NAMESPACE_END
