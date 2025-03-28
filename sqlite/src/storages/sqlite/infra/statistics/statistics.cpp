#include <userver/storages/sqlite/infra/statistics/statistics.hpp>

#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra::statistics {

void DumpMetric(utils::statistics::Writer& writer,
                const PoolStatistics& stats) {
  writer["connections"] = stats.connections;
  writer["selects"] = stats.read_operations;
  writer["mutations"] = stats.write_operations;
  writer["transactions"] = stats.transactions;
}

void DumpMetric(utils::statistics::Writer& writer,
                const PoolQueryStatistics& stats) {
  writer["total"] = stats.total;
  writer["error"] = stats.error;
  writer["timings"] = stats.timings;
}

void DumpMetric(utils::statistics::Writer& writer,
                const PoolConnectionStatistics& stats) {
  constexpr std::string_view kSQLiteConnectionPoolType = "connection_pool";
  writer["overload"].ValueWithLabels(stats.overload,
                                     {kSQLiteConnectionPoolType, stats.type});
  writer["created"].ValueWithLabels(stats.created,
                                    {kSQLiteConnectionPoolType, stats.type});
  writer["closed"].ValueWithLabels(stats.closed,
                                   {kSQLiteConnectionPoolType, stats.type});
  writer["active"].ValueWithLabels(stats.created - stats.closed,
                                   {kSQLiteConnectionPoolType, stats.type});
  writer["busy"].ValueWithLabels(stats.acquired - stats.released,
                                 {kSQLiteConnectionPoolType, stats.type});
}

void DumpMetric(utils::statistics::Writer& writer,
                const PoolTransactionStatistics& stats) {
  writer["total"] = stats.total;
  writer["commit_total"] = stats.commit_total;
  writer["rollback_total"] = stats.rollback_total;
}

}  // namespace storages::sqlite::infra::statistics

USERVER_NAMESPACE_END
