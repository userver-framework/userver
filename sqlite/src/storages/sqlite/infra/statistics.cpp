#include <userver/storages/sqlite/infra/statistics.hpp>

#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra {

void DumpMetric(utils::statistics::Writer& writer,
                const PoolStatistics& stats) {
  writer["connections"][stats.type] = stats.connections;
}

void DumpMetric(utils::statistics::Writer& writer,
                const PoolConnectionStatistics& stats) {
  writer["overload"] = stats.overload;
  writer["created"] = stats.created;
  writer["closed"] = stats.closed;

  writer["active"] = stats.created - stats.closed;
  writer["busy"] = stats.acquired - stats.released;
}

}  // namespace storages::sqlite::infra

USERVER_NAMESPACE_END
