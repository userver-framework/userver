#pragma once

#include <vector>

#include <storages/postgres/detail/time_types.hpp>

#include <utils/statistics/percentile.hpp>
#include <utils/statistics/recentperiod.hpp>
#include <utils/statistics/relaxed_counter.hpp>

namespace storages {
namespace postgres {

/// @brief Template instance statistics storage
template <typename Counter, typename Accumulator>
struct InstanceStatisticsT {
  /// Number of connections opened
  Counter conn_open_total = 0;
  /// Number of connections dropped
  Counter conn_drop_total = 0;
  /// Number of active connections
  Counter conn_active_total = 0;
  /// Number of transactions started
  Counter trx_total = 0;
  /// Number of transactions committed
  Counter trx_commit_total = 0;
  /// Number of transactions rolled back
  Counter trx_rollback_total = 0;
  /// Number of parsed queries
  Counter trx_parse_total = 0;
  /// Number of query executions
  Counter trx_execute_total = 0;
  /// Total number of replies
  Counter trx_reply_total = 0;
  /// Number of replies in binary format
  Counter trx_bin_reply_total = 0;
  /// Error during query execution
  Counter trx_error_execute_total = 0;
  /// Error during connection
  Counter conn_error_total = 0;
  /// Error caused by pool exhaustion
  Counter pool_error_exhaust_total = 0;

  // TODO pick reasonable resolution for transaction
  // execution times
  /// Transaction overall execution time distribution
  Accumulator trx_exec_percentile;
  /// Transaction delay (caused by connection creation) time distribution
  Accumulator trx_delay_percentile;
  /// Transaction overhead (spent outside of queries) time distribution
  Accumulator trx_user_percentile;
};

using Percentile = ::utils::statistics::Percentile<2048>;
using InstanceStatistics =
    InstanceStatisticsT<::utils::statistics::RelaxedCounter<uint32_t>,
                        ::utils::statistics::RecentPeriod<
                            Percentile, Percentile, detail::SteadyClock>>;

using InstanceStatisticsNonatomicBase =
    InstanceStatisticsT<uint32_t, Percentile>;

struct InstanceStatisticsNonatomic : InstanceStatisticsNonatomicBase {
  InstanceStatisticsNonatomic() = default;

  template <typename Statistics>
  InstanceStatisticsNonatomic(const Statistics& stats) {
    *this = stats;
  }
  InstanceStatisticsNonatomic(InstanceStatisticsNonatomic&&) = default;

  template <typename Statistics>
  InstanceStatisticsNonatomic& operator=(const Statistics& stats) {
    conn_open_total = stats.conn_open_total;
    conn_drop_total = stats.conn_drop_total;
    conn_active_total = stats.conn_active_total;
    trx_total = stats.trx_total;
    trx_commit_total = stats.trx_commit_total;
    trx_rollback_total = stats.trx_rollback_total;
    trx_parse_total = stats.trx_parse_total;
    trx_execute_total = stats.trx_execute_total;
    trx_reply_total = stats.trx_reply_total;
    trx_bin_reply_total = stats.trx_bin_reply_total;
    trx_error_execute_total = stats.trx_error_execute_total;
    conn_error_total = stats.conn_error_total;
    pool_error_exhaust_total = stats.pool_error_exhaust_total;

    trx_exec_percentile.Add(stats.trx_exec_percentile.GetStatsForPeriod());
    trx_delay_percentile.Add(stats.trx_delay_percentile.GetStatsForPeriod());
    trx_user_percentile.Add(stats.trx_user_percentile.GetStatsForPeriod());
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
