#pragma once

/// @file userver/storages/postgres/transaction.hpp
/// @brief @copybrief storages::postgres::Transactions

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
#include <userver/utils/trx_tracker.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

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
class Transaction {
public:
    //@{
    /** @name Shortcut transaction options constants */
    /// Read-write read committed transaction
    static constexpr TransactionOptions RW{};  // NOLINT(readability-identifier-naming)
    /// Read-only read committed transaction
    static constexpr TransactionOptions RO{TransactionOptions::kReadOnly};  // NOLINT(readability-identifier-naming)
    /// Read-only serializable deferrable transaction
    // clang-format off
    static constexpr TransactionOptions Deferrable{  // NOLINT(readability-identifier-naming)
        TransactionOptions::Deferrable()
    };
    /// Read-write repeatable read transaction
    static constexpr TransactionOptions RepeatableReadRW{  // NOLINT(readability-identifier-naming)
        storages::postgres::IsolationLevel::kRepeatableRead
    };
    /// Read-write serializable transaction
    static constexpr TransactionOptions SerializableRW{  // NOLINT(readability-identifier-naming)
        storages::postgres::IsolationLevel::kSerializable
    };
    /// Read-only repeatable read transaction
    static constexpr TransactionOptions RepeatableReadRO{  // NOLINT(readability-identifier-naming)
        storages::postgres::IsolationLevel::kRepeatableRead,
        TransactionOptions::kReadOnly
    };
    /// Read-only serializable transaction
    static constexpr TransactionOptions SerializableRO{  // NOLINT(readability-identifier-naming)
        storages::postgres::IsolationLevel::kSerializable,
        TransactionOptions::kReadOnly
    };
    // clang-format on
    //@}

    static constexpr std::size_t kDefaultRowsInChunk = 1024;

    /// @cond
    explicit Transaction(
        detail::ConnectionPtr&& conn,
        const TransactionOptions& = RW,
        OptionalCommandControl trx_cmd_ctl = {},
        detail::SteadyClock::time_point trx_start_time = detail::SteadyClock::now()
    );

    void SetName(std::string name);
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
    /// @note You may write a query in `.sql` file and generate a header file with Query from it.
    ///       See @ref scripts/docs/en/userver/sql_files.md for more information.
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
    /// @note You may write a query in `.sql` file and generate a header file with Query from it.
    ///       See @ref scripts/docs/en/userver/sql_files.md for more information.
    ///
    /// @warning Do NOT create a query string manually by embedding arguments!
    /// It leads to vulnerabilities and bad performance. Either pass arguments
    /// separately, or use storages::postgres::ParameterScope.
    template <typename... Args>
    ResultSet Execute(OptionalCommandControl statement_cmd_ctl, const Query& query, const Args&... args) {
        detail::StaticQueryParameters<sizeof...(args)> params;
        params.Write(GetConnectionUserTypes(), args...);
        return DoExecute(query, detail::QueryParameters{params}, statement_cmd_ctl);
    }

    /// Execute statement with stored parameters.
    ///
    /// Suspends coroutine for execution.
    ///
    /// @note You may write a query in `.sql` file and generate a header file with Query from it.
    ///       See @ref scripts/docs/en/userver/sql_files.md for more information.
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
    ///
    /// @note You may write a query in `.sql` file and generate a header file with Query from it.
    ///       See @ref scripts/docs/en/userver/sql_files.md for more information.
    ///
    /// @warning Do NOT create a query string manually by embedding arguments!
    /// It leads to vulnerabilities and bad performance. Either pass arguments
    /// separately, or use storages::postgres::ParameterScope.
    ResultSet Execute(OptionalCommandControl statement_cmd_ctl, const Query& query, const ParameterStore& store);

    /// Execute statement that uses an array of arguments transforming that array
    /// into N arrays of corresponding fields and executing the statement
    /// with these arrays values.
    /// Basically, a column-wise Execute.
    ///
    /// Useful for statements that unnest their arguments to avoid the need to
    /// increase timeouts due to data amount growth, but providing an explicit
    /// mapping from `Container::value_type` to PG type is infeasible for some
    /// reason (otherwise, use Execute).
    ///
    /// @note You may write a query in `.sql` file and generate a header file with Query from it.
    ///       See @ref scripts/docs/en/userver/sql_files.md for more information.
    ///
    /// @snippet storages/postgres/tests/arrays_pgtest.cpp ExecuteDecomposeTrx
    template <typename Container>
    ResultSet ExecuteDecompose(const Query& query, const Container& args);

    /// Execute statement that uses an array of arguments transforming that array
    /// into N arrays of corresponding fields and executing the statement
    /// with these arrays values.
    /// Basically, a column-wise Execute.
    ///
    /// Useful for statements that unnest their arguments to avoid the need to
    /// increase timeouts due to data amount growth, but providing an explicit
    /// mapping from `Container::value_type` to PG type is infeasible for some
    /// reason (otherwise, use Execute).
    ///
    /// @note You may write a query in `.sql` file and generate a header file with Query from it.
    ///       See @ref scripts/docs/en/userver/sql_files.md for more information.
    ///
    /// @snippet storages/postgres/tests/arrays_pgtest.cpp ExecuteDecomposeTrx
    template <typename Container>
    ResultSet ExecuteDecompose(OptionalCommandControl statement_cmd_ctl, const Query& query, const Container& args);

    /// Execute statement that uses an array of arguments splitting that array in
    /// chunks and executing the statement with a chunk of arguments.
    ///
    /// Useful for statements that unnest their arguments to avoid the need to
    /// increase timeouts due to data amount growth.
    ///
    /// @note You may write a query in `.sql` file and generate a header file with Query from it.
    ///       See @ref scripts/docs/en/userver/sql_files.md for more information.
    template <typename Container>
    void ExecuteBulk(const Query& query, const Container& args, std::size_t chunk_rows = kDefaultRowsInChunk);

    /// Execute statement that uses an array of arguments splitting that array in
    /// chunks and executing the statement with a chunk of arguments.
    ///
    /// Useful for statements that unnest their arguments to avoid the need to
    /// increase timeouts due to data amount growth.
    ///
    /// @note You may write a query in `.sql` file and generate a header file with Query from it.
    ///       See @ref scripts/docs/en/userver/sql_files.md for more information.
    template <typename Container>
    void ExecuteBulk(
        OptionalCommandControl statement_cmd_ctl,
        const Query& query,
        const Container& args,
        std::size_t chunk_rows = kDefaultRowsInChunk
    );

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
    /// @note You may write a query in `.sql` file and generate a header file with Query from it.
    ///       See @ref scripts/docs/en/userver/sql_files.md for more information.
    ///
    /// @snippet storages/postgres/tests/arrays_pgtest.cpp ExecuteDecomposeBulk
    template <typename Container>
    void ExecuteDecomposeBulk(const Query& query, const Container& args, std::size_t chunk_rows = kDefaultRowsInChunk);

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
    /// @note You may write a query in `.sql` file and generate a header file with Query from it.
    ///       See @ref scripts/docs/en/userver/sql_files.md for more information.
    ///
    /// @snippet storages/postgres/tests/arrays_pgtest.cpp ExecuteDecomposeBulk
    template <typename Container>
    void ExecuteDecomposeBulk(
        OptionalCommandControl statement_cmd_ctl,
        const Query& query,
        const Container& args,
        std::size_t chunk_rows = kDefaultRowsInChunk
    );

    /// @brief Create a portal for fetching results of a statement with arbitrary
    /// parameters.
    ///
    /// @note You may write a query in `.sql` file and generate a header file with Query from it.
    ///       See @ref scripts/docs/en/userver/sql_files.md for more information.
    template <typename... Args>
    Portal MakePortal(const Query& query, const Args&... args) {
        return MakePortal(OptionalCommandControl{}, query, args...);
    }

    /// @brief Create a portal for fetching results of a statement with arbitrary
    /// parameters and per-statement command control.
    ///
    /// @note You may write a query in `.sql` file and generate a header file with Query from it.
    ///       See @ref scripts/docs/en/userver/sql_files.md for more information.
    template <typename... Args>
    Portal MakePortal(OptionalCommandControl statement_cmd_ctl, const Query& query, const Args&... args) {
        detail::StaticQueryParameters<sizeof...(args)> params;
        params.Write(GetConnectionUserTypes(), args...);
        return MakePortal(PortalName{}, query, detail::QueryParameters{params}, statement_cmd_ctl);
    }

    /// @brief Create a portal for fetching results of a statement with stored
    /// parameters.
    ///
    /// @note You may write a query in `.sql` file and generate a header file with Query from it.
    ///       See @ref scripts/docs/en/userver/sql_files.md for more information.
    Portal MakePortal(const Query& query, const ParameterStore& store) {
        return MakePortal(OptionalCommandControl{}, query, store);
    }

    /// @brief Create a portal for fetching results of a statement with stored parameters
    /// and per-statement command control.
    ///
    /// @note You may write a query in `.sql` file and generate a header file with Query from it.
    ///       See @ref scripts/docs/en/userver/sql_files.md for more information.
    Portal MakePortal(OptionalCommandControl statement_cmd_ctl, const Query& query, const ParameterStore& store);

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
    ResultSet DoExecute(
        const Query& query,
        const detail::QueryParameters& params,
        OptionalCommandControl statement_cmd_ctl
    );
    Portal MakePortal(
        const PortalName&,
        const Query& query,
        const detail::QueryParameters& params,
        OptionalCommandControl statement_cmd_ctl
    );

    const UserTypes& GetConnectionUserTypes() const;

    std::string name_;
    detail::ConnectionPtr conn_;
    USERVER_NAMESPACE::utils::trx_tracker::TransactionLock trx_lock_;
};

template <typename Container>
ResultSet Transaction::ExecuteDecompose(const Query& query, const Container& args) {
    return io::DecomposeContainerByColumns(args).Perform([&query, this](const auto&... args) {
        return this->Execute(query, args...);
    });
}

template <typename Container>
ResultSet Transaction::ExecuteDecompose(
    OptionalCommandControl statement_cmd_ctl,
    const Query& query,
    const Container& args
) {
    return io::DecomposeContainerByColumns(args).Perform([&query, &statement_cmd_ctl, this](const auto&... args) {
        return this->Execute(statement_cmd_ctl, query, args...);
    });
}

template <typename Container>
void Transaction::ExecuteBulk(const Query& query, const Container& args, std::size_t chunk_rows) {
    auto split = io::SplitContainer(args, chunk_rows);
    for (auto&& chunk : split) {
        Execute(query, chunk);
    }
}

template <typename Container>
void Transaction::ExecuteBulk(
    OptionalCommandControl statement_cmd_ctl,
    const Query& query,
    const Container& args,
    std::size_t chunk_rows
) {
    auto split = io::SplitContainer(args, chunk_rows);
    for (auto&& chunk : split) {
        Execute(statement_cmd_ctl, query, chunk);
    }
}

template <typename Container>
void Transaction::ExecuteDecomposeBulk(const Query& query, const Container& args, std::size_t chunk_rows) {
    io::SplitContainerByColumns(args, chunk_rows).Perform([&query, this](const auto&... args) {
        this->Execute(query, args...);
    });
}

template <typename Container>
void Transaction::ExecuteDecomposeBulk(
    OptionalCommandControl statement_cmd_ctl,
    const Query& query,
    const Container& args,
    std::size_t chunk_rows
) {
    io::SplitContainerByColumns(args, chunk_rows).Perform([&query, &statement_cmd_ctl, this](const auto&... args) {
        this->Execute(statement_cmd_ctl, query, args...);
    });
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END
