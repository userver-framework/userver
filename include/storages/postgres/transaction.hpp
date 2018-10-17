#pragma once

#include <storages/postgres/postgres_fwd.hpp>
#include <storages/postgres/result_set.hpp>
#include <string>

namespace storages {
namespace postgres {

// clang-format off
/// @brief PostgreSQL transaction.
///
/// Non-copyable.
///
/// If the transaction is not explicitly finished (either committed or rolled back)
/// it will roll itself back in the destructor.
///
/// ## Usage synopsis
/// ```
/// auto trx = someCluster.Begin();
/// try {
///   auto res = trx.Execute("select col1, col2 from schema.table");
///   res = trx.Execute("update schema.table set col1 = $1 where col2 = $2", v1, v2);
///   trx.Commit();
/// } catch (some_exception) {
///   trx.Rollback();
/// }
/// ```
// clang-format on

class Transaction {
 public:
  Transaction(const Transaction&) = delete;
  Transaction& operator=(const Transaction&) = delete;

  ~Transaction();
  //@{
  /** @name Query execution */
  /// Execute statement without parameters
  /// Suspends coroutine for execution
  /// @throws NotInTransaction, SyntaxError, ConstraintViolation
  ResultSet Execute(const std::string& statement);
  /// Execute statement with arbitrary parameters
  /// Suspends coroutine for execution
  /// @throws NotInTransaction, SyntaxError, ConstraintViolation,
  /// InvalidParameterType
  template <typename... Args>
  ResultSet Execute(const std::string& statement, Args... args);
  //@}

  /// Commit the transaction
  /// Suspends coroutine until command complete.
  /// After Commit or Rollback is called, the transaction is not usable any
  /// more.
  void Commit();
  /// Rollback the transaction
  /// Suspends coroutine until command complete.
  /// After Commit or Rollback is called, the transaction is not usable any
  /// more.
  void Rollback();
};

}  // namespace postgres
}  // namespace storages
