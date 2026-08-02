#pragma once

/// @file userver/storages/odbc/transaction.hpp

#include <chrono>

#include <userver/engine/deadline.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/datetime/steady_coarse_clock.hpp>
#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/trx_tracker.hpp>

#include <userver/storages/odbc/bulk.hpp>
#include <userver/storages/odbc/command_control.hpp>
#include <userver/storages/odbc/cursor.hpp>
#include <userver/storages/odbc/impl/parameter.hpp>
#include <userver/storages/odbc/parameter_store.hpp>
#include <userver/storages/odbc/query.hpp>
#include <userver/storages/odbc/result_set.hpp>
#include <userver/storages/odbc/transaction_options.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

namespace detail {
struct BulkLayout;
class ConnectionPtr;
class Pool;
}  // namespace detail

/// @brief RAII transaction wrapper, auto-<b>ROLLBACK</b>s on destruction if no
/// prior `Commit`/`Rollback` call was made.
///
/// This type can't be constructed in user code and is always retrieved from
/// storages::odbc::Cluster
class Transaction final {
public:
    explicit Transaction(
        detail::ConnectionPtr&& connection,
        detail::Pool& pool,
        std::chrono::milliseconds network_timeout,
        std::chrono::milliseconds statement_timeout
    );
    explicit Transaction(
        detail::ConnectionPtr&& connection,
        detail::Pool& pool,
        const TransactionOptions& options,
        std::chrono::milliseconds network_timeout,
        std::chrono::milliseconds statement_timeout
    );
    ~Transaction();
    Transaction(const Transaction& other) = delete;
    Transaction(Transaction&& other) noexcept;

    /// @brief Execute a statement, binding every argument to an ODBC `?` placeholder.
    template <typename... Args>
    requires((impl::kIsParameterArgument<Args> && ...))
    ResultSet Execute(const Query& query, const Args&... args) {
        return Execute(std::nullopt, query, args...);
    }

    /// @brief Execute a statement with per-statement timeout overrides.
    template <typename... Args>
    requires((impl::kIsParameterArgument<Args> && ...))
    ResultSet Execute(OptionalCommandControl command_control, const Query& query, const Args&... args) {
        return DoExecute(command_control, query, impl::MakeParameterList(args...));
    }

    /// @brief Execute a statement with an owning dynamic parameter list.
    ResultSet Execute(const Query& query, const ParameterStore& store);

    /// @brief Execute a statement with a dynamic parameter list and timeout overrides.
    ResultSet Execute(OptionalCommandControl command_control, const Query& query, const ParameterStore& store);

    /// @brief Execute a row-producing statement as an incremental cursor.
    ///
    /// No other transaction operation is allowed until the cursor observes EOF
    /// or is destroyed.
    template <typename... Args>
    requires((impl::kIsParameterArgument<Args> && ...))
    Cursor ExecuteCursor(const Query& query, const Args&... args) {
        return ExecuteCursor(std::nullopt, query, args...);
    }

    template <typename... Args>
    requires((impl::kIsParameterArgument<Args> && ...))
    Cursor ExecuteCursor(OptionalCommandControl command_control, const Query& query, const Args&... args) {
        return DoExecuteCursor(command_control, query, impl::MakeParameterList(args...));
    }

    Cursor ExecuteCursor(const Query& query, const ParameterStore& store);

    Cursor ExecuteCursor(OptionalCommandControl command_control, const Query& query, const ParameterStore& store);

    /// Execute rows as bounded chunks of DML that must not return result sets.
    /// On failure, inspect BulkExecutionError and roll back the transaction.
    BulkResult ExecuteBulk(
        const Query& query,
        const BulkParameterStore& rows,
        std::size_t chunk_rows = kDefaultBulkRows
    );

    /// Execute bulk DML with per-operation timeout overrides.
    BulkResult ExecuteBulk(
        OptionalCommandControl command_control,
        const Query& query,
        const BulkParameterStore& rows,
        std::size_t chunk_rows = kDefaultBulkRows
    );

    /// @brief Commit the transaction
    void Commit();

    /// @brief Rollback the transaction
    void Rollback();

private:
    ResultSet DoExecute(
        OptionalCommandControl command_control,
        const Query& query,
        const impl::ParameterList& parameters
    );
    Cursor DoExecuteCursor(
        OptionalCommandControl command_control,
        const Query& query,
        const impl::ParameterList& parameters
    );
    BulkResult DoExecuteBulk(
        OptionalCommandControl command_control,
        const Query& query,
        const impl::ParameterRows& rows,
        const detail::BulkLayout& layout,
        std::size_t chunk_rows
    );
    void AssertValid() const;

    // shared_ptr<Pool>(16) + unique_ptr<Connection>(8) = 24 bytes, align 8
    utils::FastPimpl<detail::ConnectionPtr, 24, 8> connection_;
    detail::Pool* pool_;
    std::chrono::milliseconds network_timeout_;
    std::chrono::milliseconds statement_timeout_;
    utils::datetime::SteadyCoarseClock::time_point start_time_;
    std::chrono::microseconds busy_time_{0};
    tracing::Span span_;
    utils::trx_tracker::TransactionLock trx_lock_;
};

}  // namespace storages::odbc

USERVER_NAMESPACE_END
