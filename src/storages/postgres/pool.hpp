#pragma once

#include <atomic>
#include <memory>

#include <engine/task/task_processor.hpp>
#include <storages/postgres/detail/time_types.hpp>
#include <storages/postgres/dsn.hpp>
#include <storages/postgres/options.hpp>
#include <storages/postgres/transaction.hpp>
#include <utils/statistics/percentile.hpp>
#include <utils/statistics/recentperiod.hpp>
#include <utils/statistics/relaxed_counter.hpp>

namespace storages {
namespace postgres {

/// PostgreSQL client connection pool
class ConnectionPool {
 public:
  /// @brief Statistics storage
  struct Statistics {
    using CounterType = uint32_t;
    using Counter = ::utils::statistics::RelaxedCounter<CounterType>;
    using Percentile = ::utils::statistics::Percentile<2048>;
    using RecentPeriod =
        ::utils::statistics::RecentPeriod<Percentile, Percentile,
                                          detail::SteadyClock>;

    /// Number of connections opened
    Counter conn_open_total = 0;
    /// Number of connections dropped
    Counter conn_drop_total = 0;
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
    RecentPeriod trx_exec_percentile;
    /// Transaction delay (caused by connection creation) time distribution
    RecentPeriod trx_delay_percentile;
    /// Transaction overhead (spent outside of queries) time distribution
    RecentPeriod trx_user_percentile;
  };

 public:
  /// Pool constructor
  /// @param dsn server name to connect to
  /// @param bg_task_processor task processor for blocking connection operations
  /// @param initial_size initial (minimum) idle connections count
  /// @param max_size maximum idle connections count
  ConnectionPool(const std::string& dsn,
                 engine::TaskProcessor& bg_task_processor, size_t initial_size,
                 size_t max_size);
  ~ConnectionPool();

  ConnectionPool(ConnectionPool&&) noexcept;
  ConnectionPool& operator=(ConnectionPool&&);

  /// Get idle connection from pool
  /// If no idle connection and `max_size` is not reached - create a new
  /// connection.
  /// Suspends coroutine until a connection is available
  /// TODO Remove from interface
  detail::ConnectionPtr GetConnection();

  const Statistics& GetStatistics() const;

  Transaction Begin(const TransactionOptions&);

 private:
  class Impl;
  std::unique_ptr<Impl> pimpl_;
};

}  // namespace postgres
}  // namespace storages
