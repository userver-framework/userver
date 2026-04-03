#include <userver/ydb/table.hpp>

#include <userver/engine/deadline.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/statistics/writer.hpp>
#include <userver/ydb/impl/cast.hpp>

#include <ydb/impl/config.hpp>
#include <ydb/impl/driver.hpp>
#include <ydb/impl/future.hpp>
#include <ydb/impl/operation_settings.hpp>
#include <ydb/impl/request_context.hpp>
#include <ydb/impl/retry.hpp>
#include <ydb/impl/stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

namespace {

NYdb::NQuery::TTxSettings MakeTxSettings(TransactionMode tx_mode) {
    switch (tx_mode) {
        case TransactionMode::kSerializableRW:
            return NYdb::NQuery::TTxSettings::SerializableRW();
        case TransactionMode::kOnlineRO:
            return NYdb::NQuery::TTxSettings::OnlineRO();
        case TransactionMode::kStaleRO:
            return NYdb::NQuery::TTxSettings::StaleRO();
        case TransactionMode::kSnapshotRO:
            return NYdb::NQuery::TTxSettings::SnapshotRO();
        case TransactionMode::kSnapshotRW:
            return NYdb::NQuery::TTxSettings::SnapshotRW();
    }
}

NYdb::NTable::TTxSettings MakeTableTxSettings(TransactionMode tx_mode) {
    switch (tx_mode) {
        case TransactionMode::kSerializableRW:
            return NYdb::NTable::TTxSettings::SerializableRW();
        case TransactionMode::kOnlineRO:
            return NYdb::NTable::TTxSettings::OnlineRO();
        case TransactionMode::kStaleRO:
            return NYdb::NTable::TTxSettings::StaleRO();
        case TransactionMode::kSnapshotRO:
            return NYdb::NTable::TTxSettings::SnapshotRO();
        case TransactionMode::kSnapshotRW:
            return NYdb::NTable::TTxSettings::SnapshotRW();
    }
}

NYdb::NQuery::EStatsMode ConvertStatsMode(NYdb::NTable::ECollectQueryStatsMode collect_query_stats_mode) {
    switch (collect_query_stats_mode) {
        case NYdb::NTable::ECollectQueryStatsMode::None:
            return NYdb::NQuery::EStatsMode::None;
        case NYdb::NTable::ECollectQueryStatsMode::Basic:
            return NYdb::NQuery::EStatsMode::Basic;
        case NYdb::NTable::ECollectQueryStatsMode::Full:
            return NYdb::NQuery::EStatsMode::Full;
        case NYdb::NTable::ECollectQueryStatsMode::Profile:
            return NYdb::NQuery::EStatsMode::Profile;
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
      use_query_client_{settings.use_query_client},
      stats_(std::make_unique<impl::Stats>(
          settings.by_database_timings_buckets
              ? utils::span{*settings.by_database_timings_buckets}
              : impl::kDefaultPerDatabaseBounds,
          settings.by_query_timings_buckets
              ? utils::span{*settings.by_query_timings_buckets}
              : impl::kDefaultPerQueryBounds
      )),
      driver_(std::move(driver))
{
    {
        NYdb::NTable::TSessionPoolSettings session_pool_settings;
        session_pool_settings.MaxActiveSessions(settings.max_pool_size)
            .MinPoolSize(settings.min_pool_size)
            .RetryLimit(settings.get_session_retry_limit);
        NYdb::NTable::TClientSettings client_settings;
        client_settings.SessionPoolSettings(session_pool_settings);
        table_client_ = std::make_unique<NYdb::NTable::TTableClient>(driver_->GetNativeDriver(), client_settings);
        scheme_client_ = std::make_unique<NYdb::NScheme::TSchemeClient>(driver_->GetNativeDriver(), client_settings);
    }

    {
        NYdb::NQuery::TSessionPoolSettings session_pool_settings;
        session_pool_settings.MaxActiveSessions(settings.max_pool_size).MinPoolSize(settings.min_pool_size);
        NYdb::NQuery::TClientSettings client_settings;
        client_settings.SessionPoolSettings(session_pool_settings);
        query_client_ = std::make_unique<NYdb::NQuery::TQueryClient>(driver_->GetNativeDriver(), client_settings);
    }

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

template <typename FuncResult, typename QuerySettings, typename Func>
auto TableClient::ExecuteWithPathImpl(
    std::string_view path,
    std::string_view operation_name,
    OperationSettings settings,
    QuerySettings&& query_settings,
    Func&& func
) {
    using FuncArg = std::conditional_t<
        std::is_invocable_v<const Func&, NYdb::NTable::TSession, const std::string&, QuerySettings&&>,
        NYdb::NTable::TSession,
        NYdb::NTable::TTableClient&>;

    const Query query{"", Query::Name{operation_name}};
    impl::RequestContext context{*this, query, std::move(settings)};

    std::unique_ptr<FuncResult> result;

    impl::RetryOperation(
        context,
        [func = std::forward<Func>(func),
         full_path = JoinDbPath(path),
         query_settings = std::forward<QuerySettings>(query_settings),
         operation_name = std::move(operation_name),
         &context,
         &result](FuncArg arg) mutable {
            impl::ApplyToRequestSettings(query_settings, context.settings, context.deadline);
            auto future = func(std::forward<FuncArg>(arg), full_path, query_settings);
            result = std::make_unique<
                FuncResult>(impl::GetFutureValueChecked(std::move(future), operation_name, context));
        }
    );

    return std::move(*result);
}

void TableClient::BulkUpsert(
    std::string_view table,
    NYdb::TValue&& rows,
    OperationSettings settings,
    BulkUpsertSettings query_settings
) {
    ExecuteWithPathImpl<NYdb::NTable::TBulkUpsertResult>(
        table,
        "BulkUpsert",
        std::move(settings),
        std::move(query_settings),
        [rows = std::move(rows
         )](NYdb::NTable::TTableClient& table_client,
            const std::string& full_path,
            const BulkUpsertSettings& query_settings) {
            return table_client.BulkUpsert(impl::ToString(full_path), NYdb::TValue{rows}, query_settings);
        }
    );
}

ReadTableResults TableClient::ReadTable(
    std::string_view table,
    NYdb::NTable::TReadTableSettings&& read_settings,
    OperationSettings settings
) {
    const Query query{"", Query::Name{"ReadTable"}};
    impl::RequestContext context{*this, query, std::move(settings), impl::IsStreaming{true}};

    std::unique_ptr<ReadTableResults> result;

    impl::RetryOperation(
        context,
        [full_path = JoinDbPath(table),
         read_settings = std::move(read_settings),
         &context,
         &result](NYdb::NTable::TSession session) mutable {
            impl::ApplyToRequestSettings(read_settings, context.settings, context.deadline);
            auto future = session.ReadTable(impl::ToString(full_path), read_settings);
            result = std::make_unique<
                ReadTableResults>(impl::GetFutureValueChecked(std::move(future), "ReadTable", context));
        }
    );

    return std::move(*result);
}

ScanQueryResults TableClient::ExecuteScanQuery(
    ScanQuerySettings&& scan_settings,
    OperationSettings settings,
    const Query& query,
    PreparedArgsBuilder&& builder
) {
    impl::RequestContext context{*this, query, std::move(settings), impl::IsStreaming{true}};

    std::unique_ptr<ScanQueryResults> result;

    impl::RetryOperation(
        context,
        [query,
         params = std::move(builder).Build(),
         scan_settings = std::move(scan_settings),
         &context,
         &result](NYdb::NTable::TTableClient& table_client) mutable {
            impl::ApplyToRequestSettings(scan_settings, context.settings, context.deadline);
            auto future =
                table_client.StreamExecuteScanQuery(impl::ToString(query.GetStatementView()), params, scan_settings);
            result = std::make_unique<
                ScanQueryResults>(impl::GetFutureValueChecked(std::move(future), "ExecuteScanQuery", context));
        }
    );

    return std::move(*result);
}

void TableClient::Select1() {
    const auto response = ExecuteDataQuery(Query("SELECT 1")).GetSingleCursor().GetFirstRow().Get<std::int32_t>(0);
    if (response != 1) {
        throw ydb::BaseError(fmt::format("'SELECT 1' returned {}", response));
    }
}

NYdb::NTable::TTableClient& TableClient::GetNativeTableClient() { return *table_client_; }

NYdb::NQuery::TQueryClient& TableClient::GetNativeQueryClient() { return *query_client_; }

utils::RetryBudget& TableClient::GetRetryBudget() { return driver_->GetRetryBudget(); }

void TableClient::MakeDirectory(const std::string& path, MakeDirectorySettings query_settings) {
    ExecuteWithPathImpl<NYdb::TStatus>(
        path,
        "MakeDirectory",
        /*settings=*/{},
        std::move(query_settings),
        [this](NYdb::NTable::TTableClient&, const std::string& full_path, const MakeDirectorySettings& query_settings) {
            return scheme_client_->MakeDirectory(impl::ToString(full_path), query_settings);
        }
    );
}

void TableClient::RemoveDirectory(const std::string& path, RemoveDirectorySettings query_settings) {
    ExecuteWithPathImpl<NYdb::TStatus>(
        path,
        "RemoveDirectory",
        /*settings=*/{},
        std::move(query_settings),
        [this](
            NYdb::NTable::TTableClient&,
            const std::string& full_path,
            const RemoveDirectorySettings& query_settings
        ) { return scheme_client_->RemoveDirectory(impl::ToString(full_path), query_settings); }
    );
}

NYdb::NScheme::TDescribePathResult TableClient::DescribePath(
    std::string_view path,
    DescribePathSettings query_settings
) {
    return ExecuteWithPathImpl<NYdb::NScheme::TDescribePathResult>(
        path,
        "DescribePath",
        /*settings=*/{},
        std::move(query_settings),
        [this](NYdb::NTable::TTableClient&, const std::string& full_path, const DescribePathSettings& query_settings) {
            return scheme_client_->DescribePath(impl::ToString(full_path), query_settings);
        }
    );
}

NYdb::NTable::TDescribeTableResult TableClient::DescribeTable(
    std::string_view path,
    DescribeTableSettings query_settings
) {
    return ExecuteWithPathImpl<NYdb::NTable::TDescribeTableResult>(
        path,
        "DescribeTable",
        /*settings=*/{},
        std::move(query_settings),
        [](NYdb::NTable::TSession session, const std::string& full_path, const DescribeTableSettings& query_settings) {
            return session.DescribeTable(impl::ToString(full_path), query_settings);
        }
    );
}

NYdb::NScheme::TListDirectoryResult TableClient::ListDirectory(
    std::string_view path,
    ListDirectorySettings query_settings
) {
    return ExecuteWithPathImpl<NYdb::NScheme::TListDirectoryResult>(
        path,
        "ListDirectory",
        /*settings=*/{},
        std::move(query_settings),
        [this](NYdb::NTable::TTableClient&, const std::string& full_path, const ListDirectorySettings& query_settings) {
            return scheme_client_->ListDirectory(impl::ToString(full_path), query_settings);
        }
    );
}

void TableClient::CreateTable(
    std::string_view path,
    NYdb::NTable::TTableDescription&& table_desc,
    CreateTableSettings query_settings
) {
    ExecuteWithPathImpl<NYdb::TStatus>(
        path,
        "CreateTable",
        /*settings=*/{},
        std::move(query_settings),
        [table_desc = std::move(table_desc
         )](NYdb::NTable::TSession session, const std::string& full_path, const CreateTableSettings& query_settings) {
            auto table_desc_copy = table_desc;
            return session.CreateTable(impl::ToString(full_path), std::move(table_desc_copy), query_settings);
        }
    );
}

void TableClient::DropTable(std::string_view path, DropTableSettings query_settings) {
    ExecuteWithPathImpl<NYdb::TStatus>(
        path,
        "DropTable",
        /*settings=*/{},
        std::move(query_settings),
        [](NYdb::NTable::TSession session, const std::string& full_path, const DropTableSettings& query_settings) {
            return session.DropTable(impl::ToString(full_path), query_settings);
        }
    );
}

Transaction TableClient::Begin(utils::StringLiteral transaction_name, TransactionMode tx_mode) {
    OperationSettings settings{};
    settings.tx_mode = tx_mode;
    return Begin(transaction_name, std::move(settings));
}

Transaction TableClient::Begin(utils::StringLiteral transaction_name, OperationSettings settings) {
    return Begin(DynamicTransactionName{transaction_name.data()}, std::move(settings));
}

Transaction TableClient::Begin(DynamicTransactionName transaction_name, OperationSettings settings) {
    const Query query{"", Query::Name{"Begin"}};
    impl::RequestContext context{*this, query, std::move(settings)};

    std::unique_ptr<Transaction> result;

    if (use_query_client_) {
        auto tx_settings = MakeTxSettings(context.settings.tx_mode.value());

        impl::RetryQuery(
            context,
            [this,
             tx_settings = std::move(tx_settings),
             transaction_name = std::move(transaction_name),
             settings = std::move(settings),
             &context,
             &result](NYdb::NQuery::TSession session) mutable {
                const auto exec_settings = impl::PrepareRequestSettings<
                    NYdb::NQuery::TBeginTxSettings>(context.settings, context.deadline);

                auto future = session.BeginTransaction(tx_settings, exec_settings);

                result = std::make_unique<Transaction>(
                    *this,
                    impl::GetFutureValueChecked(std::move(future), "BeginTransaction", context).GetTransaction(),
                    transaction_name.GetUnderlying(),
                    std::move(settings)
                );
            }
        );
    } else {
        auto tx_settings = MakeTableTxSettings(context.settings.tx_mode.value());

        impl::RetryOperation(
            context,
            [this,
             tx_settings = std::move(tx_settings),
             transaction_name = std::move(transaction_name),
             settings = std::move(settings),
             &context,
             &result](NYdb::NTable::TSession session) mutable {
                const auto exec_settings = impl::PrepareRequestSettings<
                    NYdb::NTable::TBeginTxSettings>(context.settings, context.deadline);

                auto future = session.BeginTransaction(tx_settings, exec_settings);
                result = std::make_unique<Transaction>(
                    *this,
                    impl::GetFutureValueChecked(std::move(future), "BeginTransaction", context).GetTransaction(),
                    transaction_name.GetUnderlying(),
                    std::move(settings)
                );
            }
        );
    }

    return std::move(*result);
}

void TableClient::ExecuteSchemeQuery(const std::string& query) {
    const Query nameless_query{query};
    OperationSettings settings{};
    impl::RequestContext context{*this, nameless_query, std::move(settings)};

    impl::RetryOperation(context, [query, &context](NYdb::NTable::TSession session) {
        const auto exec_settings = impl::PrepareRequestSettings<
            NYdb::NTable::TExecSchemeQuerySettings>(context.settings, context.deadline);
        auto future = session.ExecuteSchemeQuery(impl::ToString(query), exec_settings);
        return impl::GetFutureValueChecked(std::move(future), "ExecuteSchemeQuery", context);
    });
}

ExecuteResponse TableClient::ExecuteDataQuery(
    OperationSettings settings,
    const Query& query,
    PreparedArgsBuilder&& builder
) {
    return ExecuteDataQuery(QuerySettings{}, std::move(settings), query, std::move(builder));
}

ExecuteResponse TableClient::ExecuteDataQuery(
    QuerySettings query_settings,
    OperationSettings settings,
    const Query& query,
    PreparedArgsBuilder&& builder
) {
    if (use_query_client_) {
        return ExecuteQuery(ToExecuteQuerySettings(query_settings), std::move(settings), query, std::move(builder));
    }

    impl::RequestContext context{*this, query, std::move(settings)};

    std::unique_ptr<ExecuteResponse> response;

    impl::RetryOperation(
        context,
        [query,
         params = std::move(builder).Build(),
         exec_settings = ToExecDataQuerySettings(query_settings),
         &response,
         &context](NYdb::NTable::TSession session) mutable {
            impl::ApplyToRequestSettings(exec_settings, context.settings, context.deadline);
            const auto tx_settings = MakeTableTxSettings(context.settings.tx_mode.value());
            const auto tx = NYdb::NTable::TTxControl::BeginTx(tx_settings).CommitTx();
            auto future = session.ExecuteDataQuery(impl::ToString(query.GetStatementView()), tx, params, exec_settings);
            response = std::make_unique<
                ExecuteResponse>(impl::GetFutureValueChecked(std::move(future), "ExecuteDataQuery", context));
        }
    );

    return std::move(*response);
}

ExecuteResponse TableClient::ExecuteQuery(
    OperationSettings settings,
    const Query& query,
    PreparedArgsBuilder&& builder
) {
    return ExecuteQuery(NYdb::NQuery::TExecuteQuerySettings{}, std::move(settings), query, std::move(builder));
}

ExecuteResponse TableClient::ExecuteQuery(
    NYdb::NQuery::TExecuteQuerySettings&& exec_settings,
    OperationSettings settings,
    const Query& query,
    PreparedArgsBuilder&& builder
) {
    impl::RequestContext context{*this, query, std::move(settings)};

    std::unique_ptr<ExecuteResponse> response;

    impl::RetryQuery(
        context,
        [query,
         params = std::move(builder).Build(),
         exec_settings = std::move(exec_settings),
         &response,
         &context](NYdb::NQuery::TSession session) mutable {
            impl::ApplyToRequestSettings(exec_settings, context.settings, context.deadline);
            const auto tx_settings = MakeTxSettings(context.settings.tx_mode.value());
            const auto tx = NYdb::NQuery::TTxControl::BeginTx(tx_settings).CommitTx();

            auto future = session.ExecuteQuery(impl::ToString(query.GetStatementView()), tx, params, exec_settings);
            response = std::make_unique<
                ExecuteResponse>(impl::GetFutureValueChecked(std::move(future), "ExecuteQuery", context));
        }
    );

    return std::move(*response);
}

void TableClient::RetryTx(utils::StringLiteral transaction_name, RetryTxSettings retry_settings, RetryTxFunction fn) {
    RetryTx(DynamicTransactionName{transaction_name.data()}, std::move(retry_settings), std::move(fn));
}

void TableClient::RetryTx(DynamicTransactionName transaction_name, RetryTxSettings retry_settings, RetryTxFunction fn) {
    tracing::Span span{"retry_tx"};
    span.AddTag("transaction_name", transaction_name.GetUnderlying());

    impl::StatsScope stats_scope{impl::StatsScope::TransactionTag{}, *stats_, transaction_name.GetUnderlying()};

    utils::FastScopeGuard guard([&span, &stats_scope]() noexcept {
        stats_scope.OnError();
        try {
            if (engine::current_task::ShouldCancel()) {
                stats_scope.OnCancelled();
                span.AddTag("cancelled", true);
            }
            span.AddTag(tracing::kErrorFlag, true);
        } catch (const std::exception& ex) {
            LOG_ERROR() << "Failed to mark transaction error: " << ex;
        }
    });

    impl::RetryQuery(
        retry_settings,
        *this,
        impl::GetDeadline(span, config_source_.GetSnapshot()),
        [&table_client = *this,
         fn = std::move(fn),
         tx_name = std::move(transaction_name),
         &retry_settings,
         &guard](NYdb::NQuery::TSession session, engine::Deadline deadline) mutable {
            auto begin_tx = [&table_client, retry_settings = retry_settings, &session, deadline]() mutable {
                impl::RequestContext<RetryTxSettings> begin_context{
                    table_client,
                    Query{"", Query::Name{"ydb.BeginTransaction"}},
                    std::move(retry_settings),
                    impl::IsStreaming{false},
                    nullptr,
                    deadline
                };

                const auto tx_settings = MakeTxSettings(begin_context.settings.tx_mode);
                const auto begin_tx_settings = impl::PrepareRequestSettings<
                    NYdb::NQuery::TBeginTxSettings>(begin_context.settings, begin_context.deadline);
                auto future = session.BeginTransaction(tx_settings, begin_tx_settings);
                return impl::GetFutureValueChecked(std::move(future), "BeginTransaction", begin_context)
                    .GetTransaction();
            };

            TxActor tx_actor{table_client, begin_tx(), deadline};
            auto action = fn(tx_actor);

            if (action == TxAction::kCommit) {
                const Query commit_query{"", Query::Name{"Commit"}};
                impl::RequestContext<CommitSettings> commit_context{
                    table_client,
                    commit_query,
                    CommitSettings{retry_settings.commit_settings},
                    impl::IsStreaming{false},
                    nullptr,
                    deadline
                };

                const auto commit_tx_settings = impl::PrepareRequestSettings<
                    NYdb::NQuery::TCommitTxSettings>(commit_context.settings, commit_context.deadline);
                auto commit_future = tx_actor.ydb_tx_.Commit(commit_tx_settings);
                impl::GetFutureValueChecked(std::move(commit_future), "Commit", commit_context);
                guard.Release();
            } else {
                const Query rollback_query{"", Query::Name{"Rollback"}};
                impl::RequestContext<RollbackSettings> rollback_context{
                    table_client,
                    rollback_query,
                    RollbackSettings{retry_settings.rollback_settings},
                    impl::IsStreaming{false},
                    nullptr,
                    deadline
                };

                const auto rollback_tx_settings = impl::PrepareRequestSettings<
                    NYdb::NQuery::TRollbackTxSettings>(rollback_context.settings, rollback_context.deadline);
                auto rollback_future = tx_actor.ydb_tx_.Rollback(rollback_tx_settings);
                impl::GetFutureValueChecked(std::move(rollback_future), "Rollback", rollback_context);
            }
        }
    );
}

std::string TableClient::JoinDbPath(std::string_view path) const { return impl::JoinPath(driver_->GetDbPath(), path); }

void DumpMetric(utils::statistics::Writer& writer, const TableClient& table_client) {
    writer = *table_client.stats_;

    writer["pool"]["current-size"] =
        std::max(table_client.table_client_->GetCurrentPoolSize(), table_client.query_client_->GetCurrentPoolSize());
    writer["pool"]["active-sessions"] = std::max(
        table_client.table_client_->GetActiveSessionCount(),
        table_client.query_client_->GetActiveSessionCount()
    );
    writer["pool"]["max-size"] = std::max(
        table_client.table_client_->GetActiveSessionsLimit(),
        table_client.query_client_->GetActiveSessionsLimit()
    );
}

PreparedArgsBuilder TableClient::GetBuilder() const { return PreparedArgsBuilder{}; }

NYdb::NQuery::TExecuteQuerySettings TableClient::ToExecuteQuerySettings(const QuerySettings& query_settings) const {
    NYdb::NQuery::TExecuteQuerySettings exec_settings;

    // Query Client doesn't have KeepInQueryCache, it caches automatically
    if (query_settings.collect_query_stats) {
        exec_settings.StatsMode(ConvertStatsMode(*query_settings.collect_query_stats));
    }

    return exec_settings;
}

NYdb::NTable::TExecDataQuerySettings TableClient::ToExecDataQuerySettings(const QuerySettings& query_settings) const {
    NYdb::NTable::TExecDataQuerySettings exec_settings;

    exec_settings.KeepInQueryCache(query_settings.keep_in_query_cache.value_or(keep_in_query_cache_));

    if (query_settings.collect_query_stats) {
        exec_settings.CollectQueryStats(*query_settings.collect_query_stats);
    }

    return exec_settings;
}

}  // namespace ydb

USERVER_NAMESPACE_END
