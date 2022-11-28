#include <userver/cache/lru_cache_component_base.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dump/config.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/statistics/metadata.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

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

bool IsDumpSupportEnabled(const components::ComponentConfig& config) {
  const bool dump_support_enabled = config.HasMember(dump::kDump);
  if (dump_support_enabled) {
    const auto min_interval = config[dump::kDump][dump::kMinDumpInterval];
    if (min_interval.IsMissing()) {
      throw std::runtime_error(fmt::format(
          "Missing static config field '{}'. Please fill it in explicitly. A "
          "low value (e.g. the default '0') will typically result in too "
          "frequent dump writes of the LRU cache.",
          min_interval.GetPath()));
    }
  }
  return dump_support_enabled;
}

yaml_config::Schema GetLruCacheComponentBaseSchema() {
  return yaml_config::MergeSchemas<components::LoggableComponentBase>(R"(
type: object
description: Base class for LRU-cache components
additionalProperties: false
properties:
    size:
        type: integer
        description: max amount of items to store in cache
    ways:
        type: integer
        description: number of ways for associative cache
    lifetime:
        type: string
        description: TTL for cache entries (0 is unlimited)
        defaultDescription: 0
    config-settings:
        type: boolean
        description: enables dynamic reconfiguration with CacheConfigSet
        defaultDescription: true
)");
}

}  // namespace cache::impl

USERVER_NAMESPACE_END
