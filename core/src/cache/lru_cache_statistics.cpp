#include <userver/cache/lru_cache_statistics.hpp>

#include <userver/logging/log.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache::impl {

namespace {

void WriteRateAndLegacyMetrics(utils::statistics::Writer&& writer, utils::statistics::Rate metric) {
    writer = metric.value;
    writer["v2"] = metric;
}

}  // namespace

ExpirableLruCacheStatisticsAggregator ExpirableLruCacheStatisticsBase::Load() const {
    ExpirableLruCacheStatisticsAggregator stats;
    stats.hits = hits.Load();
    stats.misses = misses.Load();
    stats.stale = stale.Load();
    stats.background_updates = background_updates.Load();
    return stats;
}

void ExpirableLruCacheStatisticsBase::Reset() {
    ResetMetric(hits);
    ResetMetric(misses);
    ResetMetric(stale);
    ResetMetric(background_updates);
}

void CacheHit(ExpirableLruCacheStatistics& stats) {
    ++stats.total.hits;
    ++stats.recent_hits.GetCurrentCounter();
    LOG_TRACE() << "cache hit";
}

void CacheMiss(ExpirableLruCacheStatistics& stats) {
    ++stats.total.misses;
    ++stats.recent_misses.GetCurrentCounter();
    LOG_TRACE() << "cache miss";
}

void CacheStale(ExpirableLruCacheStatistics& stats) {
    ++stats.total.stale;
    LOG_TRACE() << "stale cache";
}

void CacheBackgroundUpdate(ExpirableLruCacheStatistics& stats) {
    ++stats.total.background_updates;
    LOG_TRACE() << "background update";
}

void DumpMetric(utils::statistics::Writer& writer, const ExpirableLruCacheStatisticsBase& stats) {
    WriteRateAndLegacyMetrics(writer["hits"], stats.hits.Load());
    WriteRateAndLegacyMetrics(writer["misses"], stats.misses.Load());
    WriteRateAndLegacyMetrics(writer["stale"], stats.stale.Load());
    WriteRateAndLegacyMetrics(writer["background-updates"], stats.background_updates.Load());
}

void DumpMetric(utils::statistics::Writer& writer, const ExpirableLruCacheStatistics& stats) {
    writer = stats.total;

    const auto s1min_hits = stats.recent_hits.GetStatsForPeriod();
    const auto s1min_misses = stats.recent_misses.GetStatsForPeriod();
    const auto s1min_total = s1min_hits + s1min_misses;
    writer["hit_ratio"]["1min"] = static_cast<double>(s1min_hits) / static_cast<double>(s1min_total ? s1min_total : 1);
}

}  // namespace cache::impl

USERVER_NAMESPACE_END
