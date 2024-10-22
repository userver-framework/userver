#include <userver/ydb/table.hpp>

#include <userver/engine/deadline.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/statistics/writer.hpp>
#include <userver/ydb/impl/cast.hpp>

#include <ydb/impl/config.hpp>
#include <ydb/impl/driver.hpp>
#include <ydb/impl/future.hpp>
#include <ydb/impl/operation_settings.hpp>
#include <ydb/impl/request_context.hpp>
#include <ydb/impl/stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {
namespace {

NYdb::NTable::TTxSettings PrepareTxSettings(const OperationSettings& settings) {
    switch (settings.tx_mode.value()) {
        case TransactionMode::kSerializableRW: {
            return NYdb::NTable::TTxSettings::SerializableRW();
        }
        case TransactionMode::kOnlineRO: {
            return NYdb::NTable::TTxSettings::OnlineRO();
        }
        case TransactionMode::kStaleRO: {
            return NYdb::NTable::TTxSettings::StaleRO();
        }
    }
}

}  // namespace

TableClient::TableClient(
    impl::TableSettings settings,
    OperationSettings operation_settings,
    dynamic_config::Source config_source,
    std::shared_ptr<impl::Driver> driver
)
    : config_source_(config_source),
      default_settings_(std::move(operation_settings)),
      keep_in_query_cache_(settings.keep_in_query_cache),
      stats_(std::make_unique<impl::Stats>(
          settings.by_database_timings_buckets ? utils::span{*settings.by_database_timings_buckets}
                                               : impl::kDefaultPerDatabaseBounds,
          settings.by_query_timings_buckets ? utils::span{*settings.by_query_timings_buckets}
                                            : impl::kDefaultPerQueryBounds
      )),
      driver_(std::move(driver)) {
    NYdb::NTable::TSessionPoolSettings session_config;
    session_config.MaxActiveSessions(settings.max_pool_size).MinPoolSize(settings.min_pool_size);
    session_config.RetryLimit(settings.get_session_retry_limit);

    NYdb::NTable::TClientSettings client_config;
    client_config.SessionPoolSettings(session_config);

    table_client_ = std::make_unique<NYdb::NTable::TTableClient>(driver_->GetNativeDriver(), client_config);
    scheme_client_ = std::make_unique<NYdb::NScheme::TSchemeClient>(driver_->GetNativeDriver(), client_config);
    if (settings.sync_start) {
        LOG_DEBUG() << "Synchronously starting ydb client with name '" << driver_->GetDbName() << "'";
        Select1();
    }
}

TableClient::~TableClient() {
    try {
        impl::GetFutureValue(table_client_->Stop());
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error while stopping TTableClient: " << ex.what();
    }
}

template <typename Settings, typename Func>
auto TableClient::ExecuteWithPathImpl(
    std::string_view path,
    std::string_view operation_name,
    OperationSettings&& settings,
    Func&& func
) {
    using FuncArg = std::conditional_t<
        std::is_invocable_v<const Func&, NYdb::NTable::TSession, const std::string&, Settings&&>,
        NYdb::NTable::TSession,
        NYdb::NTable::TTableClient&>;

    const Query kQuery{"", Query::Name{operation_name}};
    impl::RequestContext context{*this, kQuery, settings};

    auto future = impl::RetryOperation(
        context,
        [func = std::forward<Func>(func),
         full_path = JoinDbPath(path),
         settings = std::move(settings),
         deadline = context.deadline](FuncArg arg) {
            const auto query_settings = impl::PrepareRequestSettings<Settings>(settings, deadline);
            return func(std::forward<FuncArg>(arg), full_path, query_settings);
        }
    );

    return impl::GetFutureValueChecked(std::move(future), operation_name, context);
}

void TableClient::BulkUpsert(std::string_view table, NYdb::TValue&& rows, OperationSettings settings) {
    using Settings = NYdb::NTable::TBulkUpsertSettings;
    ExecuteWithPathImpl<Settings>(
        table,
        "BulkUpsert",
        std::move(settings),
        [rows = std::move(rows)](
            NYdb::NTable::TTableClient& table_client, const std::string& full_path, const Settings& settings
        ) { return table_client.BulkUpsert(impl::ToString(full_path), NYdb::TValue{rows}, settings); }
    );
}

ReadTableResults TableClient::ReadTable(
    std::string_view table,
    NYdb::NTable::TReadTableSettings&& read_settings,
    OperationSettings settings
) {
    const Query kQuery{"", Query::Name{"ReadTable"}};
    impl::RequestContext context{*this, kQuery, settings, impl::IsStreaming{true}};

    auto future = impl::RetryOperation(
        context,
        [full_path = JoinDbPath(table),
         read_settings = std::move(read_settings),
         settings = std::move(settings),
         deadline = context.deadline](NYdb::NTable::TSession session) mutable {
            impl::ApplyToRequestSettings(read_settings, settings, deadline);
            return session.ReadTable(impl::ToString(full_path), read_settings);
        }
    );

    return ReadTableResults{impl::GetFutureValueChecked(std::move(future), "ReadTable", context)};
}

ScanQueryResults TableClient::ExecuteScanQuery(
    ScanQuerySettings&& scan_settings,
    OperationSettings settings,
    const Query& query,
    PreparedArgsBuilder&& builder
) {
    impl::RequestContext context{*this, query, settings, impl::IsStreaming{true}};

    auto future = impl::RetryOperation(
        context,
        [query = query.Statement(),
         params = std::move(builder).Build(),
         scan_settings = std::move(scan_settings),
         settings = std::move(settings),
         deadline = context.deadline](NYdb::NTable::TTableClient& table_client) mutable {
            impl::ApplyToRequestSettings(scan_settings, settings, deadline);
            return table_client.StreamExecuteScanQuery(impl::ToString(query), params, scan_settings);
        }
    );

    return ScanQueryResults{impl::GetFutureValueChecked(std::move(future), "ExecuteScanQuery", context)};
}

void TableClient::Select1() {
    const auto response = ExecuteDataQuery(Query("SELECT 1")).GetSingleCursor().GetFirstRow().Get<std::int32_t>(0);
    if (response != 1) {
        throw ydb::BaseError(fmt::format("'SELECT 1' returned {}", response));
    }
}

NYdb::NTable::TTableClient& TableClient::GetNativeTableClient() { return *table_client_; }

utils::RetryBudget& TableClient::GetRetryBudget() { return driver_->GetRetryBudget(); }

void TableClient::MakeDirectory(const std::string& path) {
    using Settings = NYdb::NScheme::TMakeDirectorySettings;
    ExecuteWithPathImpl<Settings>(
        path,
        "MakeDirectory",
        /*settings=*/{},
        [this](NYdb::NTable::TTableClient&, const std::string& full_path, const Settings& settings) {
            return scheme_client_->MakeDirectory(impl::ToString(full_path), settings);
        }
    );
}

void TableClient::RemoveDirectory(const std::string& path) {
    using Settings = NYdb::NScheme::TRemoveDirectorySettings;
    ExecuteWithPathImpl<Settings>(
        path,
        "RemoveDirectory",
        /*settings=*/{},
        [this](NYdb::NTable::TTableClient&, const std::string& full_path, const Settings& settings) {
            return scheme_client_->RemoveDirectory(impl::ToString(full_path), settings);
        }
    );
}

NYdb::NScheme::TDescribePathResult TableClient::DescribePath(std::string_view path) {
    using Settings = NYdb::NScheme::TDescribePathSettings;
    return ExecuteWithPathImpl<Settings>(
        path,
        "DescribePath",
        /*settings=*/{},
        [this](NYdb::NTable::TTableClient&, const std::string& full_path, const Settings& settings) {
            return scheme_client_->DescribePath(impl::ToString(full_path), settings);
        }
    );
}

NYdb::NTable::TDescribeTableResult TableClient::DescribeTable(std::string_view path) {
    using Settings = NYdb::NTable::TDescribeTableSettings;
    return ExecuteWithPathImpl<Settings>(
        path,
        "DescribeTable",
        /*settings=*/{},
        [](NYdb::NTable::TSession session, const std::string& full_path, const Settings& settings) {
            return session.DescribeTable(impl::ToString(full_path), settings);
        }
    );
}

NYdb::NScheme::TListDirectoryResult TableClient::ListDirectory(std::string_view path) {
    using Settings = NYdb::NScheme::TListDirectorySettings;
    return ExecuteWithPathImpl<Settings>(
        path,
        "ListDirectory",
        /*settings=*/{},
        [this](NYdb::NTable::TTableClient&, const std::string& full_path, const Settings& settings) {
            return scheme_client_->ListDirectory(impl::ToString(full_path), settings);
        }
    );
}

void TableClient::CreateTable(std::string_view path, NYdb::NTable::TTableDescription&& table_desc) {
    using Settings = NYdb::NTable::TCreateTableSettings;
    ExecuteWithPathImpl<Settings>(
        path,
        "CreateTable",
        /*settings=*/{},
        [table_desc = std::move(table_desc
         )](NYdb::NTable::TSession session, const std::string& full_path, const Settings& settings) {
            auto table_desc_copy = table_desc;
            return session.CreateTable(impl::ToString(full_path), std::move(table_desc_copy), settings);
        }
    );
}

void TableClient::DropTable(std::string_view path) {
    using Settings = NYdb::NTable::TDropTableSettings;
    ExecuteWithPathImpl<Settings>(
        path,
        "DropTable",
        /*settings=*/{},
        [](NYdb::NTable::TSession session, const std::string& full_path, const Settings& settings) {
            return session.DropTable(impl::ToString(full_path), settings);
        }
    );
}

Transaction TableClient::Begin(std::string transaction_name, TransactionMode tx_mode) {
    OperationSettings settings{};
    settings.tx_mode = tx_mode;
    return Begin(std::move(transaction_name), std::move(settings));
}

Transaction TableClient::Begin(std::string transaction_name, OperationSettings settings) {
    const Query query{"", Query::Name{"Begin"}};
    impl::RequestContext context{*this, query, settings};
    auto tx_settings = PrepareTxSettings(settings);

    auto future = impl::RetryOperation(
        context,
        [tx_settings = std::move(tx_settings), settings, deadline = context.deadline](NYdb::NTable::TSession session) {
            const auto exec_settings = impl::PrepareRequestSettings<NYdb::NTable::TBeginTxSettings>(settings, deadline);
            return session.BeginTransaction(tx_settings, exec_settings);
        }
    );

    auto status = impl::GetFutureValueChecked(std::move(future), "BeginTransaction", context);
    return Transaction(*this, status.GetTransaction(), std::move(transaction_name), std::move(settings));
}

void TableClient::ExecuteSchemeQuery(const std::string& query) {
    const Query nameless_query{query};
    OperationSettings settings{};
    impl::RequestContext context{*this, nameless_query, settings};

    auto retry_future = impl::RetryOperation(
        context,
        [query, settings = std::move(settings), deadline = context.deadline](NYdb::NTable::TSession session) {
            const auto exec_settings =
                impl::PrepareRequestSettings<NYdb::NTable::TExecSchemeQuerySettings>(settings, deadline);
            return session.ExecuteSchemeQuery(impl::ToString(query), exec_settings);
        }
    );

    impl::GetFutureValueChecked(std::move(retry_future), "ExecuteSchemeQuery", context);
}

ExecuteResponse
TableClient::ExecuteDataQuery(OperationSettings settings, const Query& query, PreparedArgsBuilder&& builder) {
    return ExecuteDataQuery(QuerySettings{}, std::move(settings), query, std::move(builder));
}

ExecuteResponse TableClient::ExecuteDataQuery(
    QuerySettings query_settings,
    OperationSettings settings,
    const Query& query,
    PreparedArgsBuilder&& builder
) {
    impl::RequestContext context{*this, query, settings};

    auto future = impl::RetryOperation(
        context,
        [query = query.Statement(),
         params = std::move(builder).Build(),
         exec_settings = ToExecQuerySettings(query_settings),
         settings = std::move(settings),
         deadline = context.deadline](NYdb::NTable::TSession session) mutable {
            impl::ApplyToRequestSettings(exec_settings, settings, deadline);
            const auto tx_settings = PrepareTxSettings(settings);
            const auto tx = NYdb::NTable::TTxControl::BeginTx(tx_settings).CommitTx();
            return session.ExecuteDataQuery(impl::ToString(query), tx, params, exec_settings);
        }
    );

    return ExecuteResponse{impl::GetFutureValueChecked(std::move(future), "ExecuteDataQuery", context)};
}

std::string TableClient::JoinDbPath(std::string_view path) const { return impl::JoinPath(driver_->GetDbPath(), path); }

void DumpMetric(utils::statistics::Writer& writer, const TableClient& table_client) {
    writer = *table_client.stats_;

    writer["pool"]["current-size"] = table_client.table_client_->GetCurrentPoolSize();
    writer["pool"]["active-sessions"] = table_client.table_client_->GetActiveSessionCount();
    writer["pool"]["max-size"] = table_client.table_client_->GetActiveSessionsLimit();
}

PreparedArgsBuilder TableClient::GetBuilder() const { return PreparedArgsBuilder(table_client_->GetParamsBuilder()); }

NYdb::NTable::TExecDataQuerySettings TableClient::ToExecQuerySettings(QuerySettings query_settings) const {
    NYdb::NTable::TExecDataQuerySettings exec_settings;
    exec_settings.KeepInQueryCache(query_settings.keep_in_query_cache.value_or(keep_in_query_cache_));
    if (query_settings.collect_query_stats) {
        exec_settings.CollectQueryStats(*query_settings.collect_query_stats);
    }
    return exec_settings;
}

}  // namespace ydb

USERVER_NAMESPACE_END
