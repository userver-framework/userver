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

}  // namespace cache::impl

USERVER_NAMESPACE_END
