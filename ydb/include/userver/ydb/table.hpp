#pragma once

#include <ydb-cpp-sdk/client/query/client.h>
#include <ydb-cpp-sdk/client/query/query.h>
#include <ydb-cpp-sdk/client/table/table.h>

#include <userver/dynamic_config/source.hpp>
#include <userver/utils/statistics/fwd.hpp>

#include <userver/ydb/builder.hpp>
#include <userver/ydb/query.hpp>
#include <userver/ydb/response.hpp>
#include <userver/ydb/settings.hpp>
#include <userver/ydb/transaction.hpp>

namespace NMonitoring {
class TMetricRegistry;
}  // namespace NMonitoring

USERVER_NAMESPACE_BEGIN

namespace tracing {
class Span;
}  // namespace tracing

namespace utils {
class RetryBudget;
}  // namespace utils

namespace ydb {

namespace impl {
struct Stats;
struct TableSettings;
class Driver;
struct RequestContext;
enum class IsStreaming : bool {};
}  // namespace impl

using DescribePathSettings = NYdb::NScheme::TDescribePathSettings;
using ListDirectorySettings = NYdb::NScheme::TListDirectorySettings;
using MakeDirectorySettings = NYdb::NScheme::TMakeDirectorySettings;
using RemoveDirectorySettings = NYdb::NScheme::TRemoveDirectorySettings;

using BulkUpsertSettings = NYdb::NTable::TBulkUpsertSettings;
using CreateTableSettings = NYdb::NTable::TCreateTableSettings;
using DescribeTableSettings = NYdb::NTable::TDescribeTableSettings;
using DropTableSettings = NYdb::NTable::TDropTableSettings;
using ScanQuerySettings = NYdb::NTable::TStreamExecScanQuerySettings;

class TableClient final {
public:
    /// @cond
    // For internal use only.
    TableClient(
        impl::TableSettings settings,
        OperationSettings operation_settings,
        dynamic_config::Source config_source,
        std::shared_ptr<impl::Driver> driver
    );

    ~TableClient();
    /// @endcond

    /// Query for creating/deleting tables
    void ExecuteSchemeQuery(const std::string& query);

    void MakeDirectory(const std::string& path, MakeDirectorySettings query_settings = {});
    void RemoveDirectory(const std::string& path, RemoveDirectorySettings query_settings = {});

    NYdb::NScheme::TDescribePathResult DescribePath(std::string_view path, DescribePathSettings query_settings = {});
    NYdb::NScheme::TListDirectoryResult ListDirectory(std::string_view path, ListDirectorySettings query_settings = {});

    NYdb::NTable::TDescribeTableResult DescribeTable(std::string_view path, DescribeTableSettings query_settings = {});
    void CreateTable(
        std::string_view path,
        NYdb::NTable::TTableDescription&& table_desc,
        CreateTableSettings query_settings = {}
    );
    void DropTable(std::string_view path, DropTableSettings query_settings = {});

    /// @name Data queries execution
    /// Execute a single data query outside of transactions. Query parameters are
    /// passed in `Args` as "string key - value" pairs:
    ///
    /// @code
    /// client.ExecuteDataQuery(query, "name1", value1, "name2", value2, ...);
    /// @endcode
    ///
    /// Use ydb::PreparedArgsBuilder for storing a generic buffer of query params
    /// if needed.
    ///
    /// @{
    template <typename... Args>
    ExecuteResponse ExecuteDataQuery(const Query& query, Args&&... args);

    template <typename... Args>
    ExecuteResponse ExecuteDataQuery(OperationSettings settings, const Query& query, Args&&... args);

    ExecuteResponse ExecuteDataQuery(OperationSettings settings, const Query& query, PreparedArgsBuilder&& builder);

    ExecuteResponse ExecuteDataQuery(
        QuerySettings query_settings,
        OperationSettings settings,
        const Query& query,
        PreparedArgsBuilder&& builder
    );
    /// @}

    /// @name Transactions
    /// @brief Begin a transaction with the specified name. The settings are used
    /// for the `BEGIN` statement.
    /// @see ydb::Transaction
    ///
    /// @{
    Transaction Begin(std::string transaction_name, OperationSettings settings = {});

    Transaction Begin(std::string transaction_name, TransactionMode tx_mode);
    /// @}

    /// Builder for storing dynamic query params.
    PreparedArgsBuilder GetBuilder() const;

    /// Efficiently write large ranges of table data.
    void BulkUpsert(
        std::string_view table,
        NYdb::TValue&& rows,
        OperationSettings settings = {},
        BulkUpsertSettings query_settings = {}
    );

    /// Efficiently write large ranges of table data.
    /// The passed range of structs is serialized to TValue.
    template <typename RangeOfStructs>
    void BulkUpsert(std::string_view table, const RangeOfStructs& rows, OperationSettings settings = {});

    /// Efficiently read large ranges of table data.
    ReadTableResults ReadTable(
        std::string_view table,
        NYdb::NTable::TReadTableSettings&& read_settings = {},
        OperationSettings settings = {}
    );

    /// @name Scan queries execution
    /// A separate data access interface designed primarily for performing
    /// analytical ad-hoc queries.
    /// @{
    template <typename... Args>
    ScanQueryResults ExecuteScanQuery(const Query& query, Args&&... args);

    template <typename... Args>
    ScanQueryResults
    ExecuteScanQuery(ScanQuerySettings&& scan_settings, OperationSettings settings, const Query& query, Args&&... args);

    ScanQueryResults ExecuteScanQuery(
        ScanQuerySettings&& scan_settings,
        OperationSettings settings,
        const Query& query,
        PreparedArgsBuilder&& builder
    );
    /// @}

    /// @name Queries execution (using YDB Query SDK)
    /// Execute a single query outside of transactions. Query parameters are
    /// passed in `Args` as "string key - value" pairs:
    ///
    /// @code
    /// client.ExecuteQuery(query, "name1", value1, "name2", value2, ...);
    /// @endcode
    ///
    /// Use ydb::PreparedArgsBuilder for storing a generic buffer of query params
    /// if needed.
    ///
    /// If both exec_settings and settings args are passed,
    /// exec_settings.client_timeout_ms and exec_settings.trace_id are ignored
    /// and are overwritten by settings.client_timeout_ms and settings.trace_id.
    /// @{
    template <typename... Args>
    ExecuteResponse ExecuteQuery(const Query& query, Args&&... args);

    template <typename... Args>
    ExecuteResponse ExecuteQuery(OperationSettings settings, const Query& query, Args&&... args);

    ExecuteResponse ExecuteQuery(OperationSettings settings, const Query& query, PreparedArgsBuilder&& builder);

    ExecuteResponse ExecuteQuery(
        NYdb::NQuery::TExecuteQuerySettings&& exec_settings,
        OperationSettings settings,
        const Query& query,
        PreparedArgsBuilder&& builder
    );
    /// @}

    /// @cond
    // For internal use only.
    friend void DumpMetric(utils::statistics::Writer& writer, const TableClient& table_client);
    /// @endcond

    /// Get native table or query client
    /// @warning Use with care! Facilities from
    /// `<core/include/userver/drivers/subscribable_futures.hpp>` can help with
    /// non-blocking wait operations.
    /// @{
    NYdb::NTable::TTableClient& GetNativeTableClient();

    NYdb::NQuery::TQueryClient& GetNativeQueryClient();
    /// @}

    utils::RetryBudget& GetRetryBudget();

private:
    friend class Transaction;
    friend struct impl::RequestContext;

    std::string JoinDbPath(std::string_view path) const;

    void Select1();

    NYdb::NTable::TExecDataQuerySettings ToExecQuerySettings(QuerySettings query_settings) const;

    template <typename... Args>
    PreparedArgsBuilder MakeBuilder(Args&&... args);

    // Func: (TSession, const std::string& full_path, const Settings&)
    //       -> NThreading::TFuture<T>
    //       OR
    //       (TTableClient&, const std::string& full_path, const Settings&)
    //       -> NThreading::TFuture<T>
    // ExecuteSchemeQueryImpl -> T
    template <typename QuerySettings, typename Func>
    auto ExecuteWithPathImpl(
        std::string_view path,
        std::string_view operation_name,
        OperationSettings settings,
        QuerySettings&& query_settings,
        Func&& func
    );

    dynamic_config::Source config_source_;
    const OperationSettings default_settings_;
    const bool keep_in_query_cache_;
    std::unique_ptr<impl::Stats> stats_;
    std::shared_ptr<impl::Driver> driver_;
    std::unique_ptr<NYdb::NScheme::TSchemeClient> scheme_client_;
    std::unique_ptr<NYdb::NTable::TTableClient> table_client_;
    std::unique_ptr<NYdb::NQuery::TQueryClient> query_client_;
};

template <typename... Args>
PreparedArgsBuilder TableClient::MakeBuilder(Args&&... args) {
    auto builder = GetBuilder();
    builder.AddParams(std::forward<Args>(args)...);
    return builder;
}

template <typename... Args>
ExecuteResponse TableClient::ExecuteDataQuery(const Query& query, Args&&... args) {
    return ExecuteDataQuery(OperationSettings{}, query, MakeBuilder(std::forward<Args>(args)...));
}

template <typename... Args>
ExecuteResponse TableClient::ExecuteDataQuery(OperationSettings settings, const Query& query, Args&&... args) {
    return ExecuteDataQuery(settings, query, MakeBuilder(std::forward<Args>(args)...));
}

template <typename RangeOfStructs>
void TableClient::BulkUpsert(std::string_view table, const RangeOfStructs& rows, OperationSettings settings) {
    NYdb::TValueBuilder builder;
    ydb::Write(builder, rows);
    BulkUpsert(table, builder.Build(), std::move(settings));
}

template <typename... Args>
ScanQueryResults TableClient::ExecuteScanQuery(const Query& query, Args&&... args) {
    return ExecuteScanQuery(ScanQuerySettings{}, OperationSettings{}, query, MakeBuilder(std::forward<Args>(args)...));
}

template <typename... Args>
ScanQueryResults TableClient::ExecuteScanQuery(
    ScanQuerySettings&& scan_settings,
    OperationSettings settings,
    const Query& query,
    Args&&... args
) {
    return ExecuteScanQuery(
        std::move(scan_settings), std::move(settings), query, MakeBuilder(std::forward<Args>(args)...)
    );
}

template <typename... Args>
ExecuteResponse TableClient::ExecuteQuery(const Query& query, Args&&... args) {
    return ExecuteQuery(OperationSettings{}, query, MakeBuilder(std::forward<Args>(args)...));
}

template <typename... Args>
ExecuteResponse TableClient::ExecuteQuery(OperationSettings settings, const Query& query, Args&&... args) {
    return ExecuteQuery(settings, query, MakeBuilder(std::forward<Args>(args)...));
}

}  // namespace ydb

USERVER_NAMESPACE_END
