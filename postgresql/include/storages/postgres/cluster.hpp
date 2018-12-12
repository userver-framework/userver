#pragma once

#include <memory>

#include <storages/postgres/cluster_types.hpp>
#include <storages/postgres/options.hpp>
#include <storages/postgres/statistics.hpp>
#include <storages/postgres/transaction.hpp>

namespace engine {
class TaskProcessor;
}  // namespace engine

namespace storages {
namespace postgres {

namespace detail {

class ClusterImpl;
using ClusterImplPtr = std::unique_ptr<ClusterImpl>;

}  // namespace detail

/// @brief Interface for executing queries on a cluster of PostgreSQL servers

class Cluster {
 public:
  /// Cluster constructor
  /// @param cluster_desc Cluster configuration description
  /// @param bg_task_processor task processor for blocking connection operations
  /// @param initial_idle_connection_pool_size initial (minimum) idle
  /// connections count
  /// @param max_connection_pool_size maximum connections count in the pool
  /// @note When `max_connection_pool_size` is reached, and no idle connections
  /// available, `PoolError` is thrown for every new connection
  /// request
  Cluster(const ClusterDescription& cluster_desc,
          engine::TaskProcessor& bg_task_processor,
          size_t initial_idle_connection_pool_size,
          size_t max_connection_pool_size);
  ~Cluster();

  /// Get cluster statistics
  ClusterStatistics GetStatistics() const;

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
 private:
  detail::ClusterImplPtr pimpl_;
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
