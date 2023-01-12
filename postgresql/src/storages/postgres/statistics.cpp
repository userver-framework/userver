#include <userver/storages/postgres/statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

void DumpMetric(USERVER_NAMESPACE::utils::statistics::Writer& writer,
                const InstanceStatisticsNonatomic& stats) {
  if (auto conn = writer["connections"]) {
    conn["opened"] = stats.connection.open_total;
    conn["closed"] = stats.connection.drop_total;
    conn["active"] = stats.connection.active;
    conn["busy"] = stats.connection.used;
    conn["max"] = stats.connection.maximum;
    conn["waiting"] = stats.connection.waiting;
    conn["max-queue-size"] = stats.connection.max_queue_size;
  }
  if (auto trx = writer["transactions"]) {
    trx["total"] = stats.transaction.total;
    trx["committed"] = stats.transaction.commit_total;
    trx["rolled-back"] = stats.transaction.rollback_total;
    trx["no-tran"] = stats.transaction.out_of_trx_total;

    auto timing = trx["timings"];
    timing["full"] = stats.transaction.total_percentile;
    timing["busy"] = stats.transaction.busy_percentile;
    timing["wait-start"] = stats.transaction.wait_start_percentile;
    timing["wait-end"] = stats.transaction.wait_end_percentile;
    timing["return-to-pool"] = stats.transaction.return_to_pool_percentile;
    timing["connect"] = stats.connection_percentile;
    timing["acquire-connection"] = stats.acquire_percentile;
  }
  if (auto query = writer["queries"]) {
    query["parsed"] = stats.transaction.parse_total;
    query["portals-bound"] = stats.transaction.portal_bind_total;
    query["executed"] = stats.transaction.execute_total;
    query["replies"] = stats.transaction.reply_total;
  }

  if (auto errors = writer["errors"]) {
    constexpr std::string_view kPostgresqlError = "postgresql_error";
    errors.ValueWithLabels(stats.transaction.error_execute_total,
                           {kPostgresqlError, "query-exec"});
    errors.ValueWithLabels(stats.transaction.execute_timeout,
                           {kPostgresqlError, "query-timeout"});
    errors.ValueWithLabels(stats.transaction.duplicate_prepared_statements,
                           {kPostgresqlError, "duplicate-prepared-statement"});
    errors.ValueWithLabels(stats.connection.error_total,
                           {kPostgresqlError, "connection"});
    errors.ValueWithLabels(stats.pool_exhaust_errors,
                           {kPostgresqlError, "pool"});
    errors.ValueWithLabels(stats.queue_size_errors,
                           {kPostgresqlError, "queue"});
    errors.ValueWithLabels(stats.connection.error_timeout,
                           {kPostgresqlError, "connection-timeout"});
  }
  writer["prepared-per-connection"] = stats.connection.prepared_statements;
  writer["roundtrip-time"] = stats.topology.roundtrip_time;
  writer["replication-lag"] = stats.topology.replication_lag;
  if (!stats.statement_timings.empty()) {
    auto timings = writer["statement_timings"];
    for (const auto& [name, percentile] : stats.statement_timings) {
      timings.ValueWithLabels(percentile, {"postgresql_query", name});
    }
  }
}

void DumpMetric(USERVER_NAMESPACE::utils::statistics::Writer& writer,
                const InstanceStatsDescriptor& value) {
  if (!value.host_port.empty()) {
    writer.ValueWithLabels(value.stats,
                           {"postgresql_instance", value.host_port});
  }
}

void DumpMetric(USERVER_NAMESPACE::utils::statistics::Writer& writer,
                const ClusterStatistics& value) {
  constexpr std::string_view kPostgresqlClusterHostType =
      "postgresql_cluster_host_type";
  writer.ValueWithLabels(value.master, {kPostgresqlClusterHostType, "master"});
  writer.ValueWithLabels(value.sync_slave,
                         {kPostgresqlClusterHostType, "sync_slave"});
  for (const auto& slave : value.slaves) {
    writer.ValueWithLabels(slave, {kPostgresqlClusterHostType, "slaves"});
  }
  for (const auto& item : value.unknown) {
    writer.ValueWithLabels(item, {kPostgresqlClusterHostType, "unknown"});
  }
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END
