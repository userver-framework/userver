#pragma once

/// @file userver/storages/postgres/transaction.hpp
/// @brief Transactions

#include <memory>
#include <string>

#include <userver/storages/postgres/detail/connection_ptr.hpp>
#include <userver/storages/postgres/detail/query_parameters.hpp>
#include <userver/storages/postgres/detail/time_types.hpp>
#include <userver/storages/postgres/options.hpp>
#include <userver/storages/postgres/parameter_store.hpp>
#include <userver/storages/postgres/portal.hpp>
#include <userver/storages/postgres/postgres_fwd.hpp>
#include <userver/storages/postgres/query.hpp>
#include <userver/storages/postgres/result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

/// @page pg_transactions uPg: Transactions
///
/// All queries that are run on a PostgreSQL cluster are executed inside
/// a transaction, even if a single-query interface is used.
///
/// A uPg transaction can be started using all isolation levels and modes
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
///
/// ----------
///
/// @htmlonly <div class="bottom-nav"> @endhtmlonly
/// ⇦ @ref pg_driver | @ref pg_run_queries ⇨
/// @htmlonly </div> @endhtmlonly

/// @page pg_run_queries uPg: Running queries
///
/// All queries are executed through a transaction object, event when being
/// executed through singe-query interface, so here only executing queries
/// with transaction will be covered. Single-query interface is basically
/// the same except for additional options.
///
/// uPg provides means to execute text queries only. There is no query
/// generation, but can be used by other tools to execute SQL queries.
///
/// @warning A query must contain a single query, multiple statements delimited
/// by ';' are not supported.
///
/// All queries are parsed and prepared during the first invocation and are
/// executed as prepared statements afterwards.
///
/// Any query execution can throw an exception. Please see @ref pg_errors for
/// more information on possible errors.
///
/// @par Queries without parameters
///
/// Executing a query without any parameters is rather straightforward.
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
/// uPg supports SQL dollar notation for parameter placeholders. The statement
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
/// @see Transaction
/// @see ResultSet
///
/// ----------
///
/// @htmlonly <div class="bottom-nav"> @endhtmlonly
/// ⇦ @ref pg_transactions | @ref pg_process_results ⇨
/// @htmlonly </div> @endhtmlonly

// clang-format off
/// @brief PostgreSQL transaction.
///
/// RAII wrapper for running transactions on PostgreSQL connections. Should be
/// retrieved by calling storages::postgres::Cluster::Begin().
///
/// Non-copyable.
///
/// If the transaction is not explicitly finished (either committed or rolled back)
/// it will roll itself back in the destructor.
///
/// @par Usage synopsis
/// @code
/// auto trx = someCluster.Begin(/* transaction options */);
/// auto res = trx.Execute("select col1, col2 from schema.table");
/// DoSomething(res);
/// res = trx.Execute("update schema.table set col1 = $1 where col2 = $2", v1, v2);
/// // If in the above lines an exception is thrown, then the transaction is
/// // rolled back in the destructor of trx.
/// trx.Commit();
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

  static constexpr std::size_t kDefaultRowsInChunk = 1024;

  /// @cond
  explicit Transaction(detail::ConnectionPtr&& conn,
                       const TransactionOptions& = RW,
                       OptionalCommandControl trx_cmd_ctl = {},
                       detail::SteadyClock::time_point trx_start_time =
                           detail::SteadyClock::now());
  /// @endcond

  Transaction(Transaction&&) noexcept;
  Transaction& operator=(Transaction&&) noexcept;

  Transaction(const Transaction&) = delete;
  Transaction& operator=(const Transaction&) = delete;

  ~Transaction();
  /// @name Query execution
  /// @{
  /// Execute statement with arbitrary parameters.
  ///
  /// Suspends coroutine for execution.
  ///
  /// @snippet storages/postgres/tests/landing_test.cpp TransacExec
  template <typename... Args>
  ResultSet Execute(const Query& query, const Args&... args) {
    return Execute(OptionalCommandControl{}, query, args...);
  }

  /// Execute statement with arbitrary parameters and per-statement command
  /// control.
  ///
  /// Suspends coroutine for execution.
  ///
  /// @warning Do NOT create a query string manually by embedding arguments!
  /// It leads to vulnerabilities and bad performance. Either pass arguments
  /// separately, or use storages::postgres::ParameterScope.
  template <typename... Args>
  ResultSet Execute(OptionalCommandControl statement_cmd_ctl,
                    const Query& query, const Args&... args) {
    detail::StaticQueryParameters<sizeof...(args)> params;
    params.Write(GetConnectionUserTypes(), args...);
    return DoExecute(query, detail::QueryParameters{params}, statement_cmd_ctl);
  }

  /// Execute statement with stored parameters.
  ///
  /// Suspends coroutine for execution.
  ///
  /// @warning Do NOT create a query string manually by embedding arguments!
  /// It leads to vulnerabilities and bad performance. Either pass arguments
  /// separately, or use storages::postgres::ParameterScope.
  ResultSet Execute(const Query& query, const ParameterStore& store) {
    return Execute(OptionalCommandControl{}, query, store);
  }

  /// Execute statement with stored parameters and per-statement command
  /// control.
  ///
  /// Suspends coroutine for execution.
  ResultSet Execute(OptionalCommandControl statement_cmd_ctl,
                    const Query& query, const ParameterStore& store);

  /// Execute statement that uses an array of arguments splitting that array in
  /// chunks and executing the statement with a chunk of arguments.
  ///
  /// Useful for statements that unnest their arguments to avoid the need to
  /// increase timeouts due to data amount growth.
  template <typename Container>
  void ExecuteBulk(const Query& query, const Container& args,
                   std::size_t chunk_rows = kDefaultRowsInChunk);

  /// Execute statement that uses an array of arguments splitting that array in
  /// chunks and executing the statement with a chunk of arguments.
  ///
  /// Useful for statements that unnest their arguments to avoid the need to
  /// increase timeouts due to data amount growth.
  template <typename Container>
  void ExecuteBulk(OptionalCommandControl statement_cmd_ctl, const Query& query,
                   const Container& args,
                   std::size_t chunk_rows = kDefaultRowsInChunk);

  /// Execute statement that uses an array of arguments transforming that array
  /// into N arrays of corresponding fields and executing the statement
  /// with a chunk of each of these arrays values.
  /// Basically, a column-wise ExecuteBulk.
  ///
  /// Useful for statements that unnest their arguments to avoid the need to
  /// increase timeouts due to data amount growth, but providing an explicit
  /// mapping from `Container::value_type` to PG type is infeasible for some
  /// reason (otherwise, use ExecuteBulk).
  ///
  /// @snippet storages/postgres/tests/arrays_pgtest.cpp ExecuteDecomposeBulk
  template <typename Container>
  void ExecuteDecomposeBulk(const Query& query, const Container& args,
                            std::size_t chunk_rows = kDefaultRowsInChunk);

  /// Execute statement that uses an array of arguments transforming that array
  /// into N arrays of corresponding fields and executing the statement
  /// with a chunk of each of these arrays values.
  /// Basically, a column-wise ExecuteBulk.
  ///
  /// Useful for statements that unnest their arguments to avoid the need to
  /// increase timeouts due to data amount growth, but providing an explicit
  /// mapping from `Container::value_type` to PG type is infeasible for some
  /// reason (otherwise, use ExecuteBulk).
  ///
  /// @snippet storages/postgres/tests/arrays_pgtest.cpp ExecuteDecomposeBulk
  template <typename Container>
  void ExecuteDecomposeBulk(OptionalCommandControl statement_cmd_ctl,
                            const Query& query, const Container& args,
                            std::size_t chunk_rows = kDefaultRowsInChunk);

  /// Create a portal for fetching results of a statement with arbitrary
  /// parameters.
  template <typename... Args>
  Portal MakePortal(const Query& query, const Args&... args) {
    return MakePortal(OptionalCommandControl{}, query, args...);
  }

  /// Create a portal for fetching results of a statement with arbitrary
  /// parameters and per-statement command control.
  template <typename... Args>
  Portal MakePortal(OptionalCommandControl statement_cmd_ctl,
                    const Query& query, const Args&... args) {
    detail::StaticQueryParameters<sizeof...(args)> params;
    params.Write(GetConnectionUserTypes(), args...);
    return MakePortal(PortalName{}, query, detail::QueryParameters{params},
                      statement_cmd_ctl);
  }

  /// Create a portal for fetching results of a statement with stored
  /// parameters.
  Portal MakePortal(const Query& query, const ParameterStore& store) {
    return MakePortal(OptionalCommandControl{}, query, store);
  }

  /// Create a portal for fetching results of a statement with stored parameters
  /// and per-statement command control.
  Portal MakePortal(OptionalCommandControl statement_cmd_ctl,
                    const Query& query, const ParameterStore& store);

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

  /// Used in tests
  OptionalCommandControl GetConnTransactionCommandControlDebug() const;
  TimeoutDuration GetConnStatementTimeoutDebug() const;

 private:
  ResultSet DoExecute(const Query& query, const detail::QueryParameters& params,
                      OptionalCommandControl statement_cmd_ctl);
  Portal MakePortal(const PortalName&, const Query& query,
                    const detail::QueryParameters& params,
                    OptionalCommandControl statement_cmd_ctl);

  const UserTypes& GetConnectionUserTypes() const;

  detail::ConnectionPtr conn_;
};

template <typename Container>
void Transaction::ExecuteBulk(const Query& query, const Container& args,
                              std::size_t chunk_rows) {
  auto split = io::SplitContainer(args, chunk_rows);
  for (auto&& chunk : split) {
    Execute(query, chunk);
  }
}

template <typename Container>
void Transaction::ExecuteBulk(OptionalCommandControl statement_cmd_ctl,
                              const Query& query, const Container& args,
                              std::size_t chunk_rows) {
  auto split = io::SplitContainer(args, chunk_rows);
  for (auto&& chunk : split) {
    Execute(statement_cmd_ctl, query, chunk);
  }
}

template <typename Container>
void Transaction::ExecuteDecomposeBulk(const Query& query,
                                       const Container& args,
                                       std::size_t chunk_rows) {
  io::SplitContainerByColumns(args, chunk_rows)
      .Perform([&query, this](const auto&... args) {
        this->Execute(query, args...);
      });
}

template <typename Container>
void Transaction::ExecuteDecomposeBulk(OptionalCommandControl statement_cmd_ctl,
                                       const Query& query,
                                       const Container& args,
                                       std::size_t chunk_rows) {
  io::SplitContainerByColumns(args, chunk_rows)
      .Perform([&query, &statement_cmd_ctl, this](const auto&... args) {
        this->Execute(statement_cmd_ctl, query, args...);
      });
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END
