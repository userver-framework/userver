#pragma once

/// @file userver/storages/postgres/statistics.hpp
/// @brief Statistics helpers

#include <unordered_map>
#include <vector>

#include <userver/storages/postgres/detail/time_types.hpp>

#include <userver/congestion_control/controllers/linear.hpp>
#include <userver/utils/statistics/min_max_avg.hpp>
#include <userver/utils/statistics/percentile.hpp>
#include <userver/utils/statistics/recentperiod.hpp>
#include <userver/utils/statistics/relaxed_counter.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

/// @brief Template transaction statistics storage
template <typename Counter, typename PercentileAccumulator>
struct TransactionStatistics {
  /// Number of transactions started
  Counter total = 0;
  /// Number of transactions committed
  Counter commit_total = 0;
  /// Number of transactions rolled back
  Counter rollback_total = 0;
  /// Number of out-of-transaction executions
  Counter out_of_trx_total = 0;
  /// Number of parsed queries
  Counter parse_total = 0;
  /// Number of query executions
  Counter execute_total = 0;
  /// Total number of replies
  Counter reply_total = 0;
  /// Number of portal bind operations
  Counter portal_bind_total{0};
  /// Error during query execution
  Counter error_execute_total = 0;
  /// Timeout while executing query
  Counter execute_timeout = 0;
  /// Duplicate prepared statements
  /// This is not a hard error, the prepared statements are quite reusable due
  /// to pretty uniqueness of names. Nevertheless we would like to see them to
  /// diagnose certain kinds of problems
  Counter duplicate_prepared_statements = 0;

  // TODO pick reasonable resolution for transaction
  // execution times
  /// Transaction overall execution time distribution
  PercentileAccumulator total_percentile;
  /// Transaction aggregated query execution time distribution
  PercentileAccumulator busy_percentile;
  /// Transaction wait for pool time (difference between trx_start_time and
  /// work_start_time)
  PercentileAccumulator wait_start_percentile;
  /// Transaction wait for pool time (difference between last_execute_finish and
  /// trx_end_time)
  PercentileAccumulator wait_end_percentile;
  /// Return to pool percentile (difference between trx_end_time and time the
  /// connection has been returned to the pool)
  PercentileAccumulator return_to_pool_percentile;
};

/// @brief Template connection statistics storage
template <typename Counter, typename MmaAccumulator>
struct ConnectionStatistics {
  /// Number of connections opened
  Counter open_total = 0;
  /// Number of connections dropped
  Counter drop_total = 0;
  /// Number of active connections
  Counter active = 0;
  /// Number of connections in use
  Counter used = 0;
  /// Number of maximum allowed connections
  Counter maximum = 0;
  /// Number of waiting requests
  Counter waiting = 0;
  /// Error during connection
  Counter error_total = 0;
  /// Connection timeouts (timeouts while connecting)
  Counter error_timeout = 0;
  /// Number of maximum allowed waiting requests
  Counter max_queue_size = 0;

  /// Prepared statements count min-max-avg
  MmaAccumulator prepared_statements;
};

/// @brief Template instance topology statistics storage
template <typename MmaAccumulator>
struct InstanceTopologyStatistics {
  /// Roundtrip time min-max-avg
  MmaAccumulator roundtrip_time;
  /// Replication lag min-max-avg
  MmaAccumulator replication_lag;
};

/// @brief Template instance statistics storage
template <typename Counter, typename PercentileAccumulator,
          typename MmaAccumulator>
struct InstanceStatisticsTemplate {
  /// Connection statistics
  ConnectionStatistics<Counter, MmaAccumulator> connection;
  /// Transaction statistics
  TransactionStatistics<Counter, PercentileAccumulator> transaction;
  /// Topology statistics
  InstanceTopologyStatistics<MmaAccumulator> topology;
  /// Error caused by pool exhaustion
  Counter pool_exhaust_errors = 0;
  /// Error caused by queue size overflow
  Counter queue_size_errors = 0;
  /// Connect time percentile
  PercentileAccumulator connection_percentile;
  /// Acquire connection percentile
  PercentileAccumulator acquire_percentile;
  /// Congestion control statistics
  std::conditional_t<std::is_same_v<Counter, uint32_t>, std::byte /* NOOP */,
                     congestion_control::v2::Stats>
      congestion_control{};
};

using Percentile = USERVER_NAMESPACE::utils::statistics::Percentile<2048>;
using MinMaxAvg = USERVER_NAMESPACE::utils::statistics::MinMaxAvg<uint32_t>;
using InstanceStatistics = InstanceStatisticsTemplate<
    USERVER_NAMESPACE::utils::statistics::RelaxedCounter<uint32_t>,
    USERVER_NAMESPACE::utils::statistics::RecentPeriod<Percentile, Percentile,
                                                       detail::SteadyClock>,
    USERVER_NAMESPACE::utils::statistics::RecentPeriod<MinMaxAvg, MinMaxAvg,
                                                       detail::SteadyClock>>;

using InstanceStatisticsNonatomicBase =
    InstanceStatisticsTemplate<uint32_t, Percentile, MinMaxAvg>;

struct InstanceStatisticsNonatomic : InstanceStatisticsNonatomicBase {
  InstanceStatisticsNonatomic() = default;

  template <typename Statistics>
  InstanceStatisticsNonatomic(const Statistics& stats) {
    *this = stats;
  }
  InstanceStatisticsNonatomic(InstanceStatisticsNonatomic&&) = default;
  InstanceStatisticsNonatomic& operator=(InstanceStatisticsNonatomic&&) =
      default;

  InstanceStatisticsNonatomic& Add(
      const InstanceStatistics& stats,
      const decltype(InstanceStatistics::topology)& topology_stats) {
    connection.open_total = stats.connection.open_total;
    connection.drop_total = stats.connection.drop_total;
    connection.active = stats.connection.active;
    connection.used = stats.connection.used;
    connection.maximum = stats.connection.maximum;
    connection.waiting = stats.connection.waiting;
    connection.error_total = stats.connection.error_total;
    connection.error_timeout = stats.connection.error_timeout;
    connection.prepared_statements =
        stats.connection.prepared_statements.GetStatsForPeriod();
    connection.max_queue_size = stats.connection.max_queue_size;

    transaction.total = stats.transaction.total;
    transaction.commit_total = stats.transaction.commit_total;
    transaction.rollback_total = stats.transaction.rollback_total;
    transaction.out_of_trx_total = stats.transaction.out_of_trx_total;
    transaction.parse_total = stats.transaction.parse_total;
    transaction.execute_total = stats.transaction.execute_total;
    transaction.reply_total = stats.transaction.reply_total;
    transaction.portal_bind_total = stats.transaction.portal_bind_total;
    transaction.error_execute_total = stats.transaction.error_execute_total;
    transaction.execute_timeout = stats.transaction.execute_timeout;
    transaction.duplicate_prepared_statements =
        stats.transaction.duplicate_prepared_statements;
    transaction.total_percentile =
        stats.transaction.total_percentile.GetStatsForPeriod();
    transaction.busy_percentile =
        stats.transaction.busy_percentile.GetStatsForPeriod();
    transaction.wait_start_percentile =
        stats.transaction.wait_start_percentile.GetStatsForPeriod();
    transaction.wait_end_percentile =
        stats.transaction.wait_end_percentile.GetStatsForPeriod();
    transaction.return_to_pool_percentile =
        stats.transaction.return_to_pool_percentile.GetStatsForPeriod();

    topology.roundtrip_time = topology_stats.roundtrip_time.GetStatsForPeriod();
    topology.replication_lag =
        topology_stats.replication_lag.GetStatsForPeriod();

    pool_exhaust_errors = stats.pool_exhaust_errors;
    queue_size_errors = stats.queue_size_errors;
    connection_percentile = stats.connection_percentile.GetStatsForPeriod();
    acquire_percentile = stats.acquire_percentile.GetStatsForPeriod();

    return *this;
  }

  InstanceStatisticsNonatomic& Add(
      const std::unordered_map<std::string, Percentile>& timings) {
    for (const auto& [name, percentile] : timings) {
      const auto [it, inserted] =
          statement_timings.try_emplace(name, percentile);
      if (!inserted) it->second.Add(percentile);
    }

    return *this;
  }

  std::unordered_map<std::string, Percentile> statement_timings;
};

/// @brief Instance statistics with description
struct InstanceStatsDescriptor {
  /// host[:port] of an instance
  std::string host_port;
  /// Statistics of an instance
  InstanceStatisticsNonatomic stats;
};

/// @brief Cluster statistics storage
struct ClusterStatistics {
  /// Master instance statistics
  InstanceStatsDescriptor master;
  /// Sync slave instance statistics
  InstanceStatsDescriptor sync_slave;
  /// Slave instances statistics
  std::vector<InstanceStatsDescriptor> slaves;
  /// Unknown/unreachable instances statistics
  std::vector<InstanceStatsDescriptor> unknown;
};

// InstanceStatisticsNonatomic values support for utils::statistics::Writer
void DumpMetric(USERVER_NAMESPACE::utils::statistics::Writer& writer,
                const InstanceStatisticsNonatomic& stats);

/// @brief InstanceStatsDescriptor values support for utils::statistics::Writer
void DumpMetric(USERVER_NAMESPACE::utils::statistics::Writer& writer,
                const InstanceStatsDescriptor& value);

/// @brief ClusterStatistics values support for utils::statistics::Writer
void DumpMetric(USERVER_NAMESPACE::utils::statistics::Writer& writer,
                const ClusterStatistics& value);

using ClusterStatisticsPtr = std::unique_ptr<ClusterStatistics>;

}  // namespace storages::postgres

USERVER_NAMESPACE_END
