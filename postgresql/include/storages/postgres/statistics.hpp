#pragma once

#include <vector>

#include <storages/postgres/detail/time_types.hpp>

#include <utils/statistics/percentile.hpp>
#include <utils/statistics/recentperiod.hpp>
#include <utils/statistics/relaxed_counter.hpp>

namespace storages {
namespace postgres {

/// @brief Template transaction statistics storage
template <typename Counter, typename Accumulator>
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
  /// Number of replies in binary format
  Counter bin_reply_total = 0;
  /// Error during query execution
  Counter error_execute_total = 0;

  // TODO pick reasonable resolution for transaction
  // execution times
  /// Transaction overall execution time distribution
  Accumulator total_percentile;
  /// Transaction aggregated query execution time distribution
  Accumulator busy_percentile;
  /// Transaction wait for pool time (difference between trx_start_time and
  /// work_start_time)
  Accumulator wait_start_percentile;
  /// Transaction wait for pool time (difference between last_execute_finish and
  /// trx_end_time)
  Accumulator wait_end_percentile;
  /// Return to pool percentile (difference between trx_end_time and time the
  /// connection has been returned to the pool)
  Accumulator return_to_pool_percentile;
};

/// @brief Template connection statistics storage
template <typename Counter>
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
  /// Error during connection
  Counter error_total = 0;
};

/// @brief Template instance statistics storage
template <typename Counter, typename Accumulator>
struct InstanceStatisticsTemplate {
  /// Connection statistics
  ConnectionStatistics<Counter> connection;
  /// Transaction statistics
  TransactionStatistics<Counter, Accumulator> transaction;
  /// Error caused by pool exhaustion
  Counter pool_error_exhaust_total = 0;
};

using Percentile = ::utils::statistics::Percentile<2048>;
using InstanceStatistics = InstanceStatisticsTemplate<
    ::utils::statistics::RelaxedCounter<uint32_t>,
    ::utils::statistics::RecentPeriod<Percentile, Percentile,
                                      detail::SteadyClock>>;

using InstanceStatisticsNonatomicBase =
    InstanceStatisticsTemplate<uint32_t, Percentile>;

struct InstanceStatisticsNonatomic : InstanceStatisticsNonatomicBase {
  InstanceStatisticsNonatomic() = default;

  template <typename Statistics>
  InstanceStatisticsNonatomic(const Statistics& stats) {
    *this = stats;
  }
  InstanceStatisticsNonatomic(InstanceStatisticsNonatomic&&) = default;

  template <typename Statistics>
  InstanceStatisticsNonatomic& operator=(const Statistics& stats) {
    connection.open_total = stats.connection.open_total;
    connection.drop_total = stats.connection.drop_total;
    connection.active = stats.connection.active;
    connection.used = stats.connection.used;
    connection.maximum = stats.connection.maximum;
    connection.error_total = stats.connection.error_total;
    transaction.total = stats.transaction.total;
    transaction.commit_total = stats.transaction.commit_total;
    transaction.rollback_total = stats.transaction.rollback_total;
    transaction.out_of_trx_total = stats.transaction.out_of_trx_total;
    transaction.parse_total = stats.transaction.parse_total;
    transaction.execute_total = stats.transaction.execute_total;
    transaction.reply_total = stats.transaction.reply_total;
    transaction.bin_reply_total = stats.transaction.bin_reply_total;
    transaction.error_execute_total = stats.transaction.error_execute_total;
    transaction.total_percentile.Add(
        stats.transaction.total_percentile.GetStatsForPeriod());
    transaction.busy_percentile.Add(
        stats.transaction.busy_percentile.GetStatsForPeriod());
    transaction.wait_start_percentile.Add(
        stats.transaction.wait_start_percentile.GetStatsForPeriod());
    transaction.wait_end_percentile.Add(
        stats.transaction.wait_end_percentile.GetStatsForPeriod());
    transaction.return_to_pool_percentile.Add(
        stats.transaction.return_to_pool_percentile.GetStatsForPeriod());
    pool_error_exhaust_total = stats.pool_error_exhaust_total;
    return *this;
  }
  InstanceStatisticsNonatomic& operator=(InstanceStatisticsNonatomic&&) =
      default;
};

/// @brief Instance statistics with description
struct InstanceStatsDescriptor {
  /// DSN of an instance
  std::string dsn;
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
};

}  // namespace postgres
}  // namespace storages
