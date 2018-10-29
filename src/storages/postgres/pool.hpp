#pragma once

#include <memory>

#include <engine/task/task_processor.hpp>
#include <storages/postgres/dsn.hpp>
#include <storages/postgres/options.hpp>
#include <storages/postgres/transaction.hpp>

namespace storages {
namespace postgres {

/// PostgreSQL client connection pool
class ConnectionPool {
 public:
  /// Pool constructor
  /// @param dsn_list list of server names to connect to
  /// @param bg_task_processor task processor for blocking connection operations
  /// @param initial_size initial (minimum) idle connections count
  /// @param max_size maximum idle connections count
  ConnectionPool(const DSNList& dsn_list,
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

  Transaction Begin(const TransactionOptions&);

 private:
  class Impl;
  std::unique_ptr<Impl> pimpl_;
};

}  // namespace postgres
}  // namespace storages
