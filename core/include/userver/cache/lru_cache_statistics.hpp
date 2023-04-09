#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>

#include <userver/utils/statistics/recentperiod.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache::impl {

struct ExpirableLruCacheStatisticsBase final {
  std::atomic<std::size_t> hits{0};
  std::atomic<std::size_t> misses{0};
  std::atomic<std::size_t> stale{0};
  std::atomic<std::size_t> background_updates{0};

  ExpirableLruCacheStatisticsBase();

  ExpirableLruCacheStatisticsBase(const ExpirableLruCacheStatisticsBase& other);

  void Reset();

  ExpirableLruCacheStatisticsBase& operator+=(
      const ExpirableLruCacheStatisticsBase& other);
};

struct ExpirableLruCacheStatistics final {
  ExpirableLruCacheStatisticsBase total;
  utils::statistics::RecentPeriod<ExpirableLruCacheStatisticsBase,
                                  ExpirableLruCacheStatisticsBase>
      recent{std::chrono::seconds(5), std::chrono::seconds(60)};
};

void CacheHit(ExpirableLruCacheStatistics& stats);

void CacheMiss(ExpirableLruCacheStatistics& stats);

void CacheStale(ExpirableLruCacheStatistics& stats);

void DumpMetric(utils::statistics::Writer& writer,
                const ExpirableLruCacheStatistics& stats);

}  // namespace cache::impl

USERVER_NAMESPACE_END
