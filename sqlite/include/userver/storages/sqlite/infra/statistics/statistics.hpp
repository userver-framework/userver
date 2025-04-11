#pragma once

#include <cstdint>

#include <userver/utils/statistics/percentile.hpp>
#include <userver/utils/statistics/recentperiod.hpp>
#include <userver/utils/statistics/relaxed_counter.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra::statistics {

using Counter = utils::statistics::RelaxedCounter<std::uint64_t>;
using Percentile = utils::statistics::Percentile<2048, uint64_t, 16, 256>;
using RecentPeriod = utils::statistics::RecentPeriod<Percentile, Percentile>;

struct PoolConnectionStatistics final {
    Counter overload{};
    Counter closed{};
    Counter created{};
    Counter acquired{};
    Counter released{};
};

struct PoolQueriesStatistics final {
    Counter total{};
    Counter error{};
    Counter executed{};
    RecentPeriod timings{};

    void Add(PoolQueriesStatistics& other);
};

struct PoolTransactionsStatistics final {
    Counter total{};
    Counter commit{};
    Counter rollback{};
    RecentPeriod timings{};

    void Add(PoolTransactionsStatistics& other);
};

struct PoolStatistics final {
    PoolConnectionStatistics connections{};
    PoolQueriesStatistics queries{};
    PoolTransactionsStatistics transactions{};
};

struct AgregatedInstanceStatistics final {
    PoolConnectionStatistics write_connections{};
    PoolConnectionStatistics read_connections{};
    const PoolQueriesStatistics& write_queries;
    const PoolQueriesStatistics& read_queries;
    const PoolTransactionsStatistics& transaction;
};

void DumpMetric(utils::statistics::Writer& writer, const AgregatedInstanceStatistics& stats);

void DumpMetric(utils::statistics::Writer& writer, const PoolQueriesStatistics& stats);

void DumpMetric(utils::statistics::Writer& writer, const PoolConnectionStatistics& stats);

void DumpMetric(utils::statistics::Writer& writer, const PoolTransactionsStatistics& stats);

}  // namespace storages::sqlite::infra::statistics

USERVER_NAMESPACE_END
