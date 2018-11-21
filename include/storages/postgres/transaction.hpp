#pragma once

#include <storages/postgres/detail/query_parameters.hpp>
#include <storages/postgres/detail/time_types.hpp>
#include <storages/postgres/options.hpp>
#include <storages/postgres/postgres_fwd.hpp>
#include <storages/postgres/result_set.hpp>

#include <memory>
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
  explicit Transaction(detail::ConnectionPtr&& impl,
                       const TransactionOptions& = TransactionOptions{},
                       detail::SteadyClock::time_point&& trx_start_time =
                           detail::SteadyClock::now());

  Transaction(Transaction&&) noexcept;
  Transaction& operator=(Transaction&&);

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
  ResultSet Execute(const std::string& statement, Args... args) {
    detail::QueryParameters params;
    params.Write(args...);
    return DoExecute(statement, params);
  }
  /// Set a connection parameter
  /// https://www.postgresql.org/docs/current/sql-set.html
  /// The parameter is set for this transaction only
  void SetParameter(const std::string& param_name, const std::string& value);
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

 private:
  ResultSet DoExecute(const std::string& statement,
                      const detail::QueryParameters& params);

 private:
  detail::ConnectionPtr conn_;
};

}  // namespace postgres
}  // namespace storages
