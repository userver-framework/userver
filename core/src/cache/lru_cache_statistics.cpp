#include <userver/cache/lru_cache_statistics.hpp>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache::impl {

ExpirableLruCacheStatisticsBase::ExpirableLruCacheStatisticsBase() = default;

ExpirableLruCacheStatisticsBase::ExpirableLruCacheStatisticsBase(
    const ExpirableLruCacheStatisticsBase& other)
    : hits(other.hits.load()),
      misses(other.misses.load()),
      stale(other.stale.load()),
      background_updates(other.background_updates.load()) {}

void ExpirableLruCacheStatisticsBase::Reset() {
  hits = 0;
  misses = 0;
  stale = 0;
  background_updates = 0;
}

ExpirableLruCacheStatisticsBase& ExpirableLruCacheStatisticsBase::operator+=(
    const ExpirableLruCacheStatisticsBase& other) {
  hits += other.hits.load();
  misses += other.misses.load();
  stale += other.stale.load();
  background_updates += other.background_updates.load();
  return *this;
}

void CacheHit(ExpirableLruCacheStatistics& stats) {
  ++stats.total.hits;
  ++stats.recent.GetCurrentCounter().hits;
  LOG_TRACE() << "cache hit";
}

void CacheMiss(ExpirableLruCacheStatistics& stats) {
  ++stats.total.misses;
  ++stats.recent.GetCurrentCounter().misses;
  LOG_TRACE() << "cache miss";
}

void CacheStale(ExpirableLruCacheStatistics& stats) {
  ++stats.total.stale;
  ++stats.recent.GetCurrentCounter().stale;
  LOG_TRACE() << "stale cache";
}

void DumpMetric(utils::statistics::Writer& writer,
                const ExpirableLruCacheStatistics& stats) {
  writer["hits"] = stats.total.hits.load();
  writer["misses"] = stats.total.misses.load();
  writer["stale"] = stats.total.stale.load();
  writer["background-updates"] = stats.total.background_updates.load();

  auto s1min = stats.recent.GetStatsForPeriod();
  double s1min_hits = s1min.hits.load();
  auto s1min_total = s1min.hits.load() + s1min.misses.load();
  writer["hit_ratio"]["1min"] =
      s1min_hits / static_cast<double>(s1min_total ? s1min_total : 1);
}

}  // namespace cache::impl

USERVER_NAMESPACE_END
