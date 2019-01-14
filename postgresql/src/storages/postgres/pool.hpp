#pragma once

#include <memory>

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

  const InstanceStatistics& GetStatistics() const;

  Transaction Begin(const TransactionOptions&);

 private:
  std::shared_ptr<detail::ConnectionPoolImpl> pimpl_;
};

}  // namespace postgres
}  // namespace storages
