#pragma once

#include <storages/postgres/options.hpp>
#include <storages/postgres/result_set.hpp>
#include <storages/postgres/transaction.hpp>

#include <string>

namespace storages {
namespace postgres {

class Connection;
using ConnectionPtr = std::shared_ptr<Connection>;

/// @brief PostreSQL connection class
/// Handles connecting to Postgres, sending commands, processing command results
/// and closing Postgres connection.
/// Responsible for all asynchronous operations
class Connection {
 public:
  using ConnectionCallback = std::function<void(ConnectionPtr)>;

 public:
  Connection(const Connection&) = delete;
  Connection(Connection&&) = delete;
  ~Connection();
  // clang-format off
  /// Connect to database using conninfo string
  ///
  /// Will suspend current coroutine
  ///
  /// @param conninfo Connection string, @see https://www.postgresql.org/docs/10/static/libpq-connect.html#LIBPQ-CONNSTRING
  /// @todo Callbacks for connection closing/becoming idle
  /// @throws ConnectionFailed
  // clang-format on
  static ConnectionPtr Connect(const std::string& conninfo,
                               ConnectionCallback on_idle = nullptr,
                               ConnectionCallback on_close = nullptr);

  /// Close the connection
  /// TODO When called from another thread/coroutine will wait for current
  /// transaction to finish.
  void Close();

  /// Check if the connection is active
  bool IsConnected() const;
  /// Check if the connection is currently idle (IsConnected &&
  /// !IsInTransaction)
  bool IsIdle() const;

  //@{
  /// Check if connection is currently in transaction
  bool IsInTransaction() const;
  /** @name Transaction interface */
  /// Begin a transaction in Postgres
  /// Suspends coroutine for execution
  /// @throws AlreadyInTransaction
  Transaction Begin(const TransactionOptions&);
  /// Commit current transaction
  /// Suspends coroutine for execution
  /// @throws NotInTransaction
  void Commit();
  /// Rollback current transaction
  /// Suspends coroutine for execution
  /// @throws NotInTransaction
  void Rollback();
  //@}

  //@{
  /** @name Command sending interface */
  /// Cancel current operation
  void Cancel();
  // TODO Define command sending interface
  ResultSet Execute(const std::string& statement);
  //@}
 private:
  Connection();

  struct Impl;
  std::unique_ptr<Impl> pimpl_;
};

}  // namespace postgres
}  // namespace storages
