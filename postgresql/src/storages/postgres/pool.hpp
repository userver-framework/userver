#pragma once

#include <memory>

#include <engine/deadline.hpp>

#include <storages/postgres/detail/non_transaction.hpp>
#include <storages/postgres/options.hpp>
#include <storages/postgres/statistics.hpp>
#include <storages/postgres/transaction.hpp>

namespace engine {
class TaskProcessor;
}

namespace storages {
namespace postgres {

namespace detail {
class ConnectionPoolImpl;
}

/// PostgreSQL client connection pool
class ConnectionPool {
 public:
  /// Pool constructor
  /// @param dsn server name to connect to
  /// @param bg_task_processor task processor for blocking connection operations
  /// @param initial_size initial (minimum) idle connections count
  /// @param max_size maximum idle connections count
  /// @param default_cmd_ctl default settings for operations
  ConnectionPool(const std::string& dsn,
                 engine::TaskProcessor& bg_task_processor, size_t initial_size,
                 size_t max_size, CommandControl default_cmd_ctl);
  ~ConnectionPool();

  ConnectionPool(ConnectionPool&&) noexcept;
  ConnectionPool& operator=(ConnectionPool&&) noexcept;

  std::string const& GetDsn() const;

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

}  // namespace postgres
}  // namespace storages
