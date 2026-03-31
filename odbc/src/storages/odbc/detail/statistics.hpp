#pragma once

#include <cstdint>

#include <userver/utils/statistics/percentile.hpp>
#include <userver/utils/statistics/recentperiod.hpp>
#include <userver/utils/statistics/relaxed_counter.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

using Counter = utils::statistics::RelaxedCounter<std::uint64_t>;
using Percentile = utils::statistics::Percentile<2048>;
using RecentPeriodPercentile = utils::statistics::RecentPeriod<Percentile, Percentile>;

struct TransactionStatistics final {
    Counter total{};
    Counter commit_total{};
    Counter rollback_total{};
    Counter out_of_trx_total{};
    Counter execute_total{};
    Counter error_execute_total{};
    Counter execute_timeout{};

    RecentPeriodPercentile total_percentile{};
    RecentPeriodPercentile busy_percentile{};
};

struct ConnectionStatistics final {
    Counter open_total{};
    Counter drop_total{};
    Counter active{};
    Counter used{};
    Counter maximum{};
    Counter waiting{};
    Counter error_total{};
    Counter error_timeout{};
    Counter max_queue_size{};
};

struct InstanceStatistics final {
    ConnectionStatistics connection{};
    TransactionStatistics transaction{};
    Counter pool_exhaust_errors{};
    Counter queue_size_errors{};

    RecentPeriodPercentile connection_percentile{};
    RecentPeriodPercentile acquire_percentile{};
};

void DumpMetric(utils::statistics::Writer& writer, const InstanceStatistics& stats);

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
