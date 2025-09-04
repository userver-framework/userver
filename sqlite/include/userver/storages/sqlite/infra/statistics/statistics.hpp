#pragma once

#include <cstdint>

#include <userver/utils/statistics/histogram.hpp>
#include <userver/utils/statistics/histogram_aggregator.hpp>
#include <userver/utils/statistics/rate_counter.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra::statistics {

using RateCounter = utils::statistics::RateCounter;

inline constexpr double kDefaultBoundsArray[] = {5, 10, 20, 35, 60, 100, 173, 300, 520};

struct PoolConnectionStatistics final {
    RateCounter overload{};
    RateCounter closed{};
    RateCounter created{};
    RateCounter acquired{};
    RateCounter released{};
};

struct PoolQueriesStatistics final {
    RateCounter total{};
    RateCounter error{};
    RateCounter executed{};
    utils::statistics::Histogram timings{kDefaultBoundsArray};
};

struct PoolTransactionsStatistics final {
    RateCounter total{};
    RateCounter commit{};
    RateCounter rollback{};
    utils::statistics::Histogram timings{kDefaultBoundsArray};
};

struct PoolTransactionsStatisticsAggregated final {
    RateCounter total{};
    RateCounter commit{};
    RateCounter rollback{};
    utils::statistics::HistogramAggregator timings{kDefaultBoundsArray};

    void Add(PoolTransactionsStatistics& other);
};

struct PoolStatistics final {
    PoolConnectionStatistics connections{};
    PoolQueriesStatistics queries{};
    PoolTransactionsStatistics transactions{};
};

struct AggregatedInstanceStatistics final {
    const PoolConnectionStatistics* write_connections{nullptr};
    const PoolConnectionStatistics* read_connections{nullptr};
    const PoolQueriesStatistics* write_queries{nullptr};
    const PoolQueriesStatistics* read_queries{nullptr};
    const PoolTransactionsStatistics* transaction{nullptr};
    const PoolTransactionsStatisticsAggregated* transaction_aggregated{nullptr};
};

void DumpMetric(utils::statistics::Writer& writer, const AggregatedInstanceStatistics& stats);

void DumpMetric(utils::statistics::Writer& writer, const PoolQueriesStatistics& stats);

void DumpMetric(utils::statistics::Writer& writer, const PoolConnectionStatistics& stats);

void DumpMetric(utils::statistics::Writer& writer, const PoolTransactionsStatistics& stats);

}  // namespace storages::sqlite::infra::statistics

USERVER_NAMESPACE_END
