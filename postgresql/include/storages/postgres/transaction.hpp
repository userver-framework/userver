#pragma once

#include <memory>
#include <string>

#include <storages/postgres/detail/connection_ptr.hpp>
#include <storages/postgres/detail/query_parameters.hpp>
#include <storages/postgres/detail/time_types.hpp>
#include <storages/postgres/options.hpp>
#include <storages/postgres/postgres_fwd.hpp>
#include <storages/postgres/result_set.hpp>

namespace storages {
namespace postgres {

/// @page pg_transactions µPg: Transactions
///
/// All queries that are run on a PostgreSQL cluster are executed inside
/// a transaction, even if a single-query interface is used.
///
/// A µPg transaction can be started using all isolation levels and modes
/// supported by PostgreSQL server as specified in documentation here
/// https://www.postgresql.org/docs/current/static/sql-set-transaction.html.
/// When starting a transaction, the options are specified using
/// TransactionOptions structure.
///
/// For convenience and improvement of readability there are constants
/// defined: Transaction::RW, Transaction::RO and Transaction::Deferrable.
///
/// @see TransactionOptions
///
/// Transaction object ensures that a transaction started in a PostgreSQL
/// connection will be either committed or rolled back and the connection
/// will returned back to a connection pool.
///
/// @todo Code snippet with transaction starting and committing
///
/// Next: @ref pg_run_queries
///
/// See also: @ref pg_process_results

/// @page pg_run_queries µPg: Running queries
///
/// All queries are executed through a transaction object, event when being
/// executed through singe-query interface, so here only executing queries
/// with transaction will be covered. Single-query interface is basically
/// the same except for additional options.
///
/// µPg provides means to execute text queries only. There is no query
/// generation, but can be used by other tools to execute SQL queries.
///
/// @warning A query must contain a single query, multiple statements delimited
/// by ';' are not supported.
///
/// All queries are parsed and prepared during the first invocation and are
/// executed as prepared afterwards.
///
/// Any query execution can throw an exception. Please see @ref pg_errors for
/// more information on possible errors.
///
/// @par Queries without parameters
///
/// Executing a query wihout any parameters is rather straightforward.
/// @code
/// auto trx = cluster->Begin(/* transaction options */);
/// auto res = trx.Execute("select foo, bar from foobar");
/// trx.Commit();
/// @endcode
///
/// The cluster also provides interface for single queries
/// @code
/// auto res = cluster->Execute(/* transaction options */, /* the statement */);
/// @endcode
///
/// @par Queries with parameters
///
/// µPg supports SQL dollar notation for parameter placeholders. The statement
/// is prepared at first execution and then only arguments for a query is sent
/// to the server.
///
/// A parameter can be of any type that is supported by the driver. See @ref
/// pg_types for more information.
///
/// @code
/// auto trx = cluster->Begin(/* transaction options */);
/// auto res = trx.Execute(
///     "select foo, bar from foobar where foo > $1 and bar = $2", 42, "baz");
/// trx.Commit();
/// @endcode
///
/// Next: @ref pg_process_results
/// @see Transaction
/// @see ResultSet

// clang-format off
/// @brief PostgreSQL transaction.
///
/// RAII wrapper for running transactions on PostgreSQL connections.
///
/// Non-copyable.
///
/// If the transaction is not explicitly finished (either committed or rolled back)
/// it will roll itself back in the destructor.
///
/// @par Usage synopsis
/// @code
/// auto trx = someCluster.Begin(/* transaction options */);
/// try {
///   auto res = trx.Execute("select col1, col2 from schema.table");
///   res = trx.Execute("update schema.table set col1 = $1 where col2 = $2", v1, v2);
///   trx.Commit();
/// } catch (some_exception) {
///   trx.Rollback();
/// }
/// @endcode
// clang-format on

class Transaction {
 public:
  //@{
  /** @name Shortcut transaction options constants */
  /// Read-write read committed transaction
  static constexpr TransactionOptions RW{};
  /// Read-only read committed transaction
  static constexpr TransactionOptions RO{TransactionOptions::kReadOnly};
  /// Read-only serializable deferrable transaction
  static constexpr TransactionOptions Deferrable{
      TransactionOptions::Deferrable()};
  //@}
 public:
  explicit Transaction(detail::ConnectionPtr&& conn,
                       const TransactionOptions& = RW,
                       OptionalCommandControl trx_cmd_ctl = {},
                       detail::SteadyClock::time_point&& trx_start_time =
                           detail::SteadyClock::now());

  Transaction(Transaction&&) noexcept;
  Transaction& operator=(Transaction&&) noexcept;

  Transaction(const Transaction&) = delete;
  Transaction& operator=(const Transaction&) = delete;

  ~Transaction();
  //@{
  /** @name Query execution */
  /// Execute statement with arbitrary parameters
  /// Suspends coroutine for execution
  /// @throws NotInTransaction, SyntaxError, ConstraintViolation,
  /// InvalidParameterType
  template <typename... Args>
  ResultSet Execute(const std::string& statement, Args const&... args) {
    detail::QueryParameters params;
    params.Write(GetConnectionUserTypes(), args...);
    return DoExecute(statement, params, {});
  }
  /// Execute statement with arbitrary parameters and per-statement command
  /// control.
  /// Suspends coroutine for execution
  /// @throws NotInTransaction, SyntaxError, ConstraintViolation,
  /// InvalidParameterType
  template <typename... Args>
  ResultSet Execute(CommandControl statement_cmd_ctl,
                    const std::string& statement, Args const&... args) {
    detail::QueryParameters params;
    params.Write(GetConnectionUserTypes(), args...);
    return DoExecute(statement, params, std::move(statement_cmd_ctl));
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
                      const detail::QueryParameters& params,
                      OptionalCommandControl statement_cmd_ctl);
  const UserTypes& GetConnectionUserTypes() const;

 private:
  detail::ConnectionPtr conn_;
};

}  // namespace postgres
}  // namespace storages
