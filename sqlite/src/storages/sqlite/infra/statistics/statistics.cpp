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
  writer[stats.type]["overload"] = stats.overload;
  writer[stats.type]["created"] = stats.created;
  writer[stats.type]["closed"] = stats.closed;

  writer[stats.type]["active"] = stats.created - stats.closed;
  writer[stats.type]["busy"] = stats.acquired - stats.released;
}

void DumpMetric(utils::statistics::Writer& writer,
                const PoolTransactionStatistics& stats) {
  writer["total"] = stats.total;
  writer["commit_total"] = stats.commit_total;
  writer["rollback_total"] = stats.rollback_total;
}

}  // namespace storages::sqlite::infra::statistics

USERVER_NAMESPACE_END
