#pragma once

#include <userver/utils/statistics/percentile.hpp>
#include <userver/utils/statistics/rate_counter.hpp>
#include <userver/utils/statistics/recentperiod.hpp>
#include <userver/utils/statistics/relaxed_counter.hpp>

#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::stats {

using Counter = USERVER_NAMESPACE::utils::statistics::RelaxedCounter<uint64_t>;
using RateCounter = USERVER_NAMESPACE::utils::statistics::RateCounter;
using Percentile = USERVER_NAMESPACE::utils::statistics::Percentile<2048, uint64_t, 16, 256>;
using RecentPeriod = USERVER_NAMESPACE::utils::statistics::RecentPeriod<Percentile, Percentile>;

struct PoolConnectionStatistics final {
    RateCounter overload{};
    RateCounter closed{};
    RateCounter created{};
    Counter active{};
    Counter busy{};
};

struct PoolQueryStatistics final {
    RateCounter total{};
    RateCounter error{};
    RecentPeriod timings{};
};

struct PoolStatistics final {
    PoolConnectionStatistics connections{};
    PoolQueryStatistics queries{};
    PoolQueryStatistics inserts{};
};

void DumpMetric(USERVER_NAMESPACE::utils::statistics::Writer& writer, const PoolStatistics& stats);

void DumpMetric(USERVER_NAMESPACE::utils::statistics::Writer& writer, const PoolQueryStatistics& stats);

void DumpMetric(USERVER_NAMESPACE::utils::statistics::Writer& writer, const PoolConnectionStatistics& stats);

}  // namespace storages::clickhouse::stats

USERVER_NAMESPACE_END
