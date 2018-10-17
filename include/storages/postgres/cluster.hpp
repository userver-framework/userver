#pragma once

#include <storages/postgres/options.hpp>
#include <storages/postgres/transaction.hpp>

namespace storages {
namespace postgres {

enum class ClusterHostType {
  /// Connect to cluster's master. Only this connection may be
  /// used for read-write transactions.
  kMaster = 0x01,
  /// Connect to cluster's sync slave. May fallback to master. Can be used only
  /// for read only transactions.
  kSyncSlave = 0x02,
  /// Connect to one of cluster's slaves. May fallback to sync
  /// slave.  Can be used only for read only transactions.
  kSlave = 0x04,
  /// Any available
  kAny = kMaster | kSyncSlave | kSlave
};

/// @brief Interface for executing queries on a cluster of PostgreSQL servers

class Cluster {
 public:
  //@{
  /** @name Transaction start */
  /// Start a transaction in any available connection depending on transaction
  /// options. If the transaction is RW, will start transaction in a connection
  /// to master. If the transaction is RO, will start trying connections
  /// starting with slaves.
  /// @throws ClusterUnavailable if no hosts are available
  Transaction Begin(const TransactionOptions&);
  /// Start a transaction in a connection to a specific cluster part
  /// If specified host type is not available, may fall back to other host type,
  /// @see ClusterHostType.
  /// If the transaction is RW, only master connection will be used.
  /// @throws ClusterUnavailable if no hosts are available
  Transaction Begin(ClusterHostType, const TransactionOptions&);
  //@}

  //@{
  /** @name Single-statement query in an auto-commit transaction */
  //@{
  /** @name Parameterless */
  /// Execute the statement in any available connection, @see Begin for
  /// connection selection logic.
  /// @throws ClusterUnavailable, SyntaxError, ConstraintViolation
  ResultSet Execute(const TransactionOptions&, const std::string& statement);
  /// Start a transaction in a connection to a specific cluster part
  /// @see Begin
  /// @throws ClusterUnavailable, SyntaxError, ConstraintViolation
  ResultSet Execute(ClusterHostType, const TransactionOptions&,
                    const std::string& statement);
  //@}
  //@{
  /** @name With arbitrary parameters */
  // clang-format off
  /// Execute the statement in any available connection
  /// @throws ClusterUnavailable, SyntaxError, ConstraintViolation, InvalidParameterType
  // clang-format on
  template <typename... Args>
  ResultSet Execute(const TransactionOptions&, const std::string& statement,
                    Args... args);
  // clang-format off
  /// Start a transaction in a connection to a specific cluster part
  /// @throws ClusterUnavailable, SyntaxError, ConstraintViolation, InvalidParameterType
  // clang-format on
  template <typename... Args>
  ResultSet Execute(ClusterHostType, const TransactionOptions&,
                    const std::string& statement, Args... args);
  //@}
  //@}
};

template <typename... Args>
ResultSet Cluster::Execute(const TransactionOptions& opts,
                           const std::string& statement, Args... args) {
  auto trx = Begin(opts);
  auto res = trx.Execute(statement, std::forward<Args>(args)...);
  trx.Commit();
  return res;
}

template <typename... Args>
ResultSet Cluster::Execute(ClusterHostType ht, const TransactionOptions& opts,
                           const std::string& statement, Args... args) {
  auto trx = Begin(ht, opts);
  auto res = trx.Execute(statement, std::forward<Args>(args)...);
  trx.Commit();
  return res;
}

}  // namespace postgres
}  // namespace storages
