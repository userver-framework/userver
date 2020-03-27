#pragma once

#include <memory>

#include <engine/deadline.hpp>
#include <error_injection/settings_fwd.hpp>

#include <storages/postgres/detail/non_transaction.hpp>
#include <storages/postgres/options.hpp>
#include <storages/postgres/statistics.hpp>
#include <storages/postgres/transaction.hpp>
#include <testsuite/postgres_control.hpp>

namespace engine {
class TaskProcessor;
}  // namespace engine

namespace storages::postgres {

namespace detail {
class ConnectionPoolImpl;
}  // namespace detail

/// PostgreSQL client connection pool
class ConnectionPool {
 public:
  /// Pool constructor
  /// @param dsn server name to connect to
  /// @param bg_task_processor task processor for blocking connection operations
  /// @param initial_size initial (minimum) idle connections count
  /// @param max_size maximum idle connections count
  /// @param default_cmd_ctl default settings for operations
  /// @param testsuite_pg_ctl operation settings customizer for testsuite
  /// @param ei_settings error injection settings
  ConnectionPool(Dsn dsn, engine::TaskProcessor& bg_task_processor,
                 const PoolSettings& pool_settings,
                 const ConnectionSettings& conn_settings,
                 const CommandControl& default_cmd_ctl,
                 const testsuite::PostgresControl& testsuite_pg_ctl,
                 error_injection::Settings ei_settings);
  ~ConnectionPool();

  ConnectionPool(ConnectionPool&&) noexcept;
  ConnectionPool& operator=(ConnectionPool&&) noexcept;

  /// Get idle connection from pool
  /// If no idle connection and `max_size` is not reached - create a new
  /// connection.
  /// Suspends coroutine until a connection is available
  /// TODO Remove from interface
  detail::ConnectionPtr GetConnection(engine::Deadline);

  const InstanceStatistics& GetStatistics() const;

  [[nodiscard]] Transaction Begin(const TransactionOptions&,
                                  engine::Deadline deadline,
                                  OptionalCommandControl = {});

  [[nodiscard]] detail::NonTransaction Start(engine::Deadline deadline);

  void SetDefaultCommandControl(CommandControl);

 private:
  std::shared_ptr<detail::ConnectionPoolImpl> pimpl_;
};

}  // namespace storages::postgres
