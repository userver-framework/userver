#include <userver/storages/sqlite/infra/statistics/statistics.hpp>

#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra::statistics {

void PoolQueriesStatistics::Add(PoolQueriesStatistics& other) {
    total += other.total;
    executed += other.executed;
    error += other.error;
    timings.GetCurrentCounter().Account(other.timings.GetCurrentCounter().Count());
}

void PoolTransactionsStatistics::Add(PoolTransactionsStatistics& other) {
    total += other.total;
    commit += other.commit;
    rollback += other.rollback;
    timings.GetCurrentCounter().Account(other.timings.GetCurrentCounter().Count());
}

void DumpMetric(utils::statistics::Writer& writer, const AgregatedInstanceStatistics& stats) {
    if (auto connections_writer = writer["connections"]) {
        connections_writer.ValueWithLabels(stats.write_connections, {{"connection_pool", "write"}});
        connections_writer.ValueWithLabels(stats.read_connections, {{"connection_pool", "read"}});
    }

    if (auto connections_writer = writer["queries"]) {
        connections_writer.ValueWithLabels(stats.write_queries, {{"connection_pool", "write"}});
        connections_writer.ValueWithLabels(stats.read_queries, {{"connection_pool", "read"}});
    }

    if (auto transactions_writer = writer["transactions"]) {
        transactions_writer = stats.transaction;
    }
}

void DumpMetric(utils::statistics::Writer& writer, const PoolConnectionStatistics& stats) {
    writer["overload"] = stats.overload;
    writer["created"] = stats.created;
    writer["closed"] = stats.closed;
    writer["active"] = stats.created - stats.closed;
    writer["busy"] = stats.acquired - stats.released;
}

void DumpMetric(utils::statistics::Writer& writer, const PoolQueriesStatistics& stats) {
    writer["total"] = stats.total;
    writer["executed"] = stats.executed;
    writer["error"] = stats.error;
    writer["timings"] = stats.timings;
}

void DumpMetric(utils::statistics::Writer& writer, const PoolTransactionsStatistics& stats) {
    writer["total"] = stats.total;
    writer["commit"] = stats.commit;
    writer["rollback"] = stats.rollback;
    writer["timings"] = stats.timings;
}

}  // namespace storages::sqlite::infra::statistics

USERVER_NAMESPACE_END
