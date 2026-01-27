#include <userver/storages/sqlite/infra/statistics/statistics.hpp>

#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra::statistics {

namespace {
template <class T>
void DumpAggregatedTransactionMetric(utils::statistics::Writer& writer, const T& stats) {
    writer["total"] = stats.total;
    writer["commit"] = stats.commit;
    writer["rollback"] = stats.rollback;
    writer["timings"] = stats.timings;
}
}  // namespace

void PoolTransactionsStatisticsAggregated::Add(PoolTransactionsStatistics& other) {
    total += other.total;
    commit += other.commit;
    rollback += other.rollback;
    timings.Add(other.timings.GetView());
}

void DumpMetric(utils::statistics::Writer& writer, const AggregatedInstanceStatistics& stats) {
    if (auto connections_writer = writer["connections"]) {
        if (stats.write_connections) {
            connections_writer.ValueWithLabels(*stats.write_connections, {{"connection_pool", "write"}});
        }
        if (stats.read_connections) {
            connections_writer.ValueWithLabels(*stats.read_connections, {{"connection_pool", "read"}});
        }
    }

    if (auto connections_writer = writer["queries"]) {
        if (stats.write_queries) {
            connections_writer.ValueWithLabels(*stats.write_queries, {{"connection_pool", "write"}});
        }
        if (stats.read_queries) {
            connections_writer.ValueWithLabels(*stats.read_queries, {{"connection_pool", "read"}});
        }
    }

    if (auto transactions_writer = writer["transactions"]) {
        if (stats.transaction) {
            UASSERT(!stats.transaction_aggregated);
            transactions_writer = *stats.transaction;
        } else {
            UASSERT(stats.transaction_aggregated);
            transactions_writer = *stats.transaction_aggregated;
        }
    }
}

void DumpMetric(utils::statistics::Writer& writer, const PoolConnectionStatistics& stats) {
    writer["overload"] = stats.overload;
    writer["created"] = stats.created;
    writer["closed"] = stats.closed;
    writer["active"] = stats.created.Load().value - stats.closed.Load().value;
    writer["busy"] = stats.acquired.Load().value - stats.released.Load().value;
}

void DumpMetric(utils::statistics::Writer& writer, const PoolQueriesStatistics& stats) {
    writer["total"] = stats.total;
    writer["executed"] = stats.executed;
    writer["error"] = stats.error;
    writer["timings"] = stats.timings;
}

void DumpMetric(utils::statistics::Writer& writer, const PoolTransactionsStatistics& stats) {
    DumpAggregatedTransactionMetric(writer, stats);
}

void DumpMetric(utils::statistics::Writer& writer, const PoolTransactionsStatisticsAggregated& stats) {
    DumpAggregatedTransactionMetric(writer, stats);
}

}  // namespace storages::sqlite::infra::statistics

USERVER_NAMESPACE_END
