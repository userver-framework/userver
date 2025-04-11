#pragma once

#include <userver/utils/statistics/fwd.hpp>
#include <userver/utils/statistics/rate.hpp>
#include <userver/utils/statistics/rate_counter.hpp>
#include <userver/utils/statistics/recentperiod.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache::impl {

struct ExpirableLruCacheStatisticsAggregator final {
    utils::statistics::Rate hits;
    utils::statistics::Rate misses;
    utils::statistics::Rate stale;
    utils::statistics::Rate background_updates;

    ExpirableLruCacheStatisticsAggregator& operator+=(const ExpirableLruCacheStatisticsAggregator& other);
};

struct ExpirableLruCacheStatisticsBase final {
    utils::statistics::RateCounter hits;
    utils::statistics::RateCounter misses;
    utils::statistics::RateCounter stale;
    utils::statistics::RateCounter background_updates;

    ExpirableLruCacheStatisticsAggregator Load() const;

    void Reset();
};

struct ExpirableLruCacheStatistics final {
    ExpirableLruCacheStatisticsBase total;
    utils::statistics::RecentPeriod<std::atomic<std::uint64_t>, std::uint64_t> recent_hits;
    utils::statistics::RecentPeriod<std::atomic<std::uint64_t>, std::uint64_t> recent_misses;
};

void CacheHit(ExpirableLruCacheStatistics& stats);

void CacheMiss(ExpirableLruCacheStatistics& stats);

void CacheStale(ExpirableLruCacheStatistics& stats);

void CacheBackgroundUpdate(ExpirableLruCacheStatistics& stats);

void DumpMetric(utils::statistics::Writer& writer, const ExpirableLruCacheStatisticsBase& stats);

void DumpMetric(utils::statistics::Writer& writer, const ExpirableLruCacheStatistics& stats);

}  // namespace cache::impl

USERVER_NAMESPACE_END
