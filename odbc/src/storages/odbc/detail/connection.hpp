#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

#include <userver/cache/lru_map.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#include <userver/storages/odbc/bulk.hpp>
#include <userver/storages/odbc/command_control.hpp>
#include <userver/storages/odbc/impl/parameter.hpp>
#include <userver/storages/odbc/query.hpp>
#include <userver/storages/odbc/result_set.hpp>
#include <userver/storages/odbc/transaction_options.hpp>

#include <storages/odbc/detail/bulk.hpp>
#include <storages/odbc/detail/cursor_state.hpp>
#include <storages/odbc/detail/driver_capabilities.hpp>
#include <storages/odbc/detail/prepared_statement_cache.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

namespace detail {
class BrokenGuard;
class CommandControlStore;
}  // namespace detail

/// @brief ODBC connection wrapper
class Connection final {
public:
    using EnvironmentHandle = std::unique_ptr<std::remove_pointer_t<SQLHENV>, void (*)(SQLHENV)>;
    using DatabaseHandle = std::unique_ptr<std::remove_pointer_t<SQLHDBC>, void (*)(SQLHDBC)>;

    explicit Connection(const std::string& dsn);
    Connection(const std::string& dsn, engine::Deadline deadline);
    Connection(
        const std::string& dsn,
        engine::TaskProcessor& blocking_task_processor,
        engine::Deadline deadline,
        std::shared_ptr<detail::PreparedStatementCacheState> prepared_statement_cache_state = nullptr
    );

    ~Connection();

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    /// @brief Executes a SQL query and returns the result set
    /// @param query SQL query to execute
    /// @return ResultSet containing the query results
    ResultSet Query(std::string_view query);

    /// @brief Executes a prepared SQL query with separately bound parameters.
    ResultSet Query(std::string_view query, const impl::ParameterList& parameters);

    /// @brief Same as Query(std::string_view), but honours \a deadline for wait / driver timeout.
    ResultSet Query(std::string_view query, engine::Deadline deadline);

    ResultSet Query(std::string_view query, const impl::ParameterList& parameters, engine::Deadline deadline);

    ResultSet Query(
        const storages::odbc::Query& query,
        const impl::ParameterList& parameters,
        engine::Deadline deadline
    );

    BulkResult QueryBulk(
        const storages::odbc::Query& query,
        const impl::ParameterRows& rows,
        const detail::BulkLayout& layout,
        std::size_t chunk_rows,
        engine::Deadline deadline
    );

    // required by ConnectionPool
    bool IsBroken() const;
    bool IsMarkedBroken() const noexcept;
    void NotifyBroken();

    detail::BrokenGuard GetBrokenGuard();

    /// Internal per-HDBC metadata snapshot captured during construction.
    const detail::DriverCapabilities& GetDriverCapabilities() const noexcept;

    void SetCommandControlStore(std::shared_ptr<detail::CommandControlStore> store);
    CommandControl ResolveTransactionCommandControl(
        CommandControl transaction_base,
        const storages::odbc::Query& query,
        OptionalCommandControl explicit_command_control
    ) const;

    /// @cond
    bool HasActiveCursor() const noexcept;
    void InvalidateActiveCursor() noexcept;
    std::chrono::microseconds TakeCursorTransactionBusyTime() noexcept;
    detail::CursorLease OpenCursor(
        const storages::odbc::Query& query,
        const impl::ParameterList& parameters,
        engine::Deadline deadline,
        bool in_transaction,
        std::function<void(detail::CursorTerminalResult, std::chrono::microseconds)> on_terminal
    );
    static ResultSet FetchCursor(detail::CursorLease& lease, std::size_t rows, engine::Deadline deadline);
    static void CloseCursor(
        detail::CursorLease& lease,
        detail::CursorTerminalResult terminal_result = detail::CursorTerminalResult::kSuccess
    ) noexcept;
    /// @endcond

private:
    struct StatementHandleDeleter final {
        Connection* connection{nullptr};
        void operator()(std::remove_pointer_t<SQLHSTMT>* handle) const noexcept;
    };
    using StatementHandle = std::unique_ptr<std::remove_pointer_t<SQLHSTMT>, StatementHandleDeleter>;

    struct CachedStatementMetadata final {
        std::optional<bool> row_producing;
        bool bulk_dml_validated{false};
    };

    struct CachedStatement final {
        CachedStatement(std::string key, StatementHandle handle, CachedStatementMetadata metadata);
        CachedStatement(CachedStatement&&) noexcept = default;
        CachedStatement& operator=(CachedStatement&&) noexcept = default;

        std::string key;
        StatementHandle handle;
        CachedStatementMetadata metadata;
    };

    using PreparedStatements = cache::LruMap<std::string, CachedStatement>;

    struct TransactionAttributes final {
        std::optional<SQLUINTEGER> isolation;
        std::optional<SQLUINTEGER> access_mode;
        std::optional<SQLUINTEGER> autocommit;
    };

    friend class Transaction;
    void Begin(const TransactionOptions& options, engine::Deadline deadline);
    void Commit(engine::Deadline deadline);
    void Rollback(engine::Deadline deadline);
    bool IsInsideTransaction() const noexcept;

    StatementHandle MakeStatementHandle();
    StatementHandle TakePreparedStatement(std::string_view query, CachedStatementMetadata* metadata = nullptr);
    void StorePreparedStatement(
        std::string query,
        StatementHandle handle,
        bool was_cached,
        std::size_t operation_reset_generation,
        CachedStatementMetadata metadata
    );
    void ApplyPreparedStatementCacheSettings();
    void ClearPreparedStatements(bool account_evictions) noexcept;
    void EvictLeastRecentlyUsedPreparedStatement() noexcept;
    void AccountPreparedStatementFailure(bool was_cached) noexcept;
    bool InvalidatesPreparedStatements(SQLSMALLINT completion_type) const noexcept;
    void DestroyStatementHandle(SQLHSTMT handle) noexcept;

    void EndTransaction(SQLSMALLINT completion_type, std::string_view operation, engine::Deadline deadline);
    void RestoreTransactionAttributes(const TransactionAttributes& attributes, engine::Deadline deadline);
    void CleanupInterruptedBegin() noexcept;
    void UpdateBrokenFromDriver() noexcept;
    engine::TaskProcessor& blocking_task_processor_;
    mutable std::mutex handle_mutex_;
    EnvironmentHandle env_;
    DatabaseHandle handle_;
    detail::DriverCapabilities driver_capabilities_;
    std::shared_ptr<detail::PreparedStatementCacheState> prepared_statement_cache_state_;
    PreparedStatements prepared_statements_{1};
    std::size_t applied_prepared_cache_size_{0};
    std::size_t applied_prepared_cache_reset_generation_{0};
    std::shared_ptr<detail::CommandControlStore> command_control_store_;
    std::shared_ptr<detail::CursorControl> cursor_control_;
    std::optional<TransactionAttributes> transaction_attributes_snapshot_;
    std::atomic<bool> broken_{false};
    std::atomic<bool> in_transaction_{false};
};

}  // namespace storages::odbc

USERVER_NAMESPACE_END
