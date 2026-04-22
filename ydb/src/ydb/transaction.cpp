#include <userver/ydb/transaction.hpp>

#include <userver/formats/json/inline.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/overloaded.hpp>

#include <userver/ydb/impl/cast.hpp>
#include <userver/ydb/table.hpp>

#include <ydb/impl/config.hpp>
#include <ydb/impl/driver.hpp>
#include <ydb/impl/future.hpp>
#include <ydb/impl/operation_settings.hpp>
#include <ydb/impl/request_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

TxActor::TxActor(
    TableClient& table_client,
    NYdb::NQuery::TSession& session,
    NYdb::NQuery::TTxSettings&& tx_settings,
    engine::Deadline deadline,
    std::uint32_t attempt
) noexcept
    : table_client_(table_client),
      deadline_(deadline),
      attempt_(attempt),
      ydb_tx_(BeginTx(session, std::move(tx_settings)))
{}

NYdb::NQuery::TTransaction TxActor::BeginTx(NYdb::NQuery::TSession& session, NYdb::NQuery::TTxSettings&& tx_settings) {
    impl::RequestContext<RequestSettings> context{
        table_client_,
        Query{"", Query::Name{"Begin"}},
        RequestSettings{},
        impl::IsStreaming{false},
        nullptr,
        deadline_
    };
    context.span.AddTag("attempt", attempt_);

    const auto ydb_settings = impl::PrepareRequestSettings<
        NYdb::NQuery::TBeginTxSettings>(context.settings, context.deadline, context.span.GetTraceId());
    auto future = session.BeginTransaction(tx_settings, ydb_settings);
    return impl::GetFutureValueChecked(std::move(future), "BeginTransaction", context).GetTransaction();
}

ExecuteResponse TxActor::Execute(ExecuteSettings settings, const Query& query, PreparedArgsBuilder&& builder) {
    impl::RequestContext<ExecuteSettings>
        context{table_client_, query, std::move(settings), impl::IsStreaming{false}, nullptr, deadline_};
    context.span.AddTag("attempt", attempt_);

    auto internal_params = std::move(builder).Build();

    auto exec_settings = impl::PrepareRequestSettings<
        NYdb::NQuery::TExecuteQuerySettings,
        ExecuteSettings>(context.settings, context.deadline, context.span.GetTraceId());

    const auto tx = NYdb::NQuery::TTxControl::Tx(ydb_tx_);
    auto execute_fut =
        ydb_tx_.GetSession()
            .ExecuteQuery(impl::ToString(query.GetStatementView()), tx, std::move(internal_params), exec_settings);

    auto status = impl::GetFutureValueChecked(std::move(execute_fut), "TxActor::Execute", context);

    return ExecuteResponse(std::move(status));
}

template <TxAction Action>
void TxActor::FinishTx(const RequestSettings& settings) {
    constexpr std::string_view action_name = Action == TxAction::kCommit ? "Commit" : "Rollback";

    impl::RequestContext<RequestSettings> context{
        table_client_,
        Query{"", Query::Name{action_name}},
        RequestSettings{settings},
        impl::IsStreaming{false},
        nullptr,
        deadline_
    };
    context.span.AddTag("attempt", attempt_);

    using SettingsType = std::conditional_t<
        Action == TxAction::kCommit,
        NYdb::NQuery::TCommitTxSettings,
        NYdb::NQuery::TRollbackTxSettings>;

    const auto ydb_settings = impl::PrepareRequestSettings<
        SettingsType>(context.settings, context.deadline, context.span.GetTraceId());

    if constexpr (Action == TxAction::kCommit) {
        auto future = ydb_tx_.Commit(ydb_settings);
        impl::GetFutureValueChecked(std::move(future), action_name, context);
    } else {
        auto future = ydb_tx_.Rollback(ydb_settings);
        impl::GetFutureValueChecked(std::move(future), action_name, context);
    }
}

template void TxActor::FinishTx<TxAction::kCommit>(const RequestSettings& settings);
template void TxActor::FinishTx<TxAction::kRollback>(const RequestSettings& settings);

PreparedArgsBuilder TxActor::GetBuilder() const { return table_client_.GetBuilder(); }

Transaction::Transaction(
    TableClient& table_client,
    std::variant<NYdb::NQuery::TTransaction, NYdb::NTable::TTransaction> ydb_tx,
    std::string name,
    OperationSettings&& rollback_settings
) noexcept
    : table_client_(table_client),
      name_(std::move(name)),
      stats_scope_(impl::StatsScope::TransactionTag{}, *table_client_.stats_, name_),
      span_("ydb_transaction"),
      ydb_tx_(std::move(ydb_tx)),
      rollback_settings_(std::move(rollback_settings)) {
    span_.DetachFromCoroStack();
    span_.AddTag("transaction_name", name_);
    trx_lock_.Lock();
}

Transaction::~Transaction() {
    if (is_active_) {
        try {
            Rollback();
        } catch (const std::exception& e) {
            LOG_WARNING() << "Failed to automatically ROLLBACK: " << e;
        }
    }
}

void Transaction::MarkError() noexcept {
    UASSERT(is_active_);
    is_active_ = false;
    stats_scope_.OnError();
    try {
        if (engine::current_task::ShouldCancel()) {
            stats_scope_.OnCancelled();
            span_.AddTag("cancelled", true);
        }
        span_.AddTag(tracing::kErrorFlag, true);
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Failed to mark transaction error: " << ex;
    }
}

auto Transaction::ErrorGuard() {
    return utils::FastScopeGuard([this]() noexcept { MarkError(); });
}

void Transaction::Commit(OperationSettings settings) {
    EnsureActive();

    static const Query kQuery{"", Query::Name{"Commit"}};
    impl::RequestContext context{table_client_, kQuery, std::move(settings), impl::IsStreaming{false}, &span_};

    if (!name_.empty()) {
        TESTPOINT_CALLBACK(
            "ydb_trx_commit",
            formats::json::MakeObject("trx_name", name_),
            [this](const formats::json::Value& data) {
                if (data["trx_should_fail"].As<bool>()) {
                    LOG_WARNING()
                        << "Doing Rollback instead of commit "
                           "due to Testpoint response";
                    std::visit([](auto&& tx) { tx.Rollback(); }, ydb_tx_);
                    throw TransactionForceRollback();
                }
            }
        );
    }

    std::visit(
        [this, &context](auto&& tx) {
            using SettingsType = std::conditional_t<
                std::is_same_v<std::decay_t<decltype(tx)>, NYdb::NQuery::TTransaction>,
                NYdb::NQuery::TCommitTxSettings,
                NYdb::NTable::TCommitTxSettings>;

            const auto commit_settings = impl::PrepareRequestSettings<SettingsType>(context.settings, context.deadline);

            [[maybe_unused]] auto error_guard = ErrorGuard();

            impl::GetFutureValueChecked(
                tx.Commit(commit_settings),
                "Commit",
                table_client_.driver_->GetRetryBudget(),
                context
            );

            error_guard.Release();
            is_active_ = false;
            trx_lock_.Unlock();
        },
        ydb_tx_
    );
}

void Transaction::Rollback() {
    EnsureActive();

    static const Query kQuery{"", Query::Name{"Rollback"}};
    auto settings = rollback_settings_;
    impl::RequestContext context{table_client_, kQuery, std::move(settings), impl::IsStreaming{false}, &span_};

    std::visit(
        [this, &context](auto&& tx) {
            using SettingsType = std::conditional_t<
                std::is_same_v<std::decay_t<decltype(tx)>, NYdb::NQuery::TTransaction>,
                NYdb::NQuery::TRollbackTxSettings,
                NYdb::NTable::TRollbackTxSettings>;

            const auto
                rollback_settings = impl::PrepareRequestSettings<SettingsType>(context.settings, context.deadline);

            [[maybe_unused]] auto error_guard = ErrorGuard();

            impl::GetFutureValueChecked(
                tx.Rollback(rollback_settings),
                "Rollback",
                table_client_.driver_->GetRetryBudget(),
                context
            );

            trx_lock_.Unlock();

            // Successful rollback is still a transaction error for logs and stats.
        },
        ydb_tx_
    );
}

PreparedArgsBuilder Transaction::GetBuilder() const { return PreparedArgsBuilder{}; }

void Transaction::EnsureActive() const {
    if (!is_active_) {
        throw InvalidTransactionError();
    }
}

ExecuteResponse Transaction::Execute(OperationSettings settings, const Query& query, PreparedArgsBuilder&& builder) {
    return Execute(QuerySettings{}, std::move(settings), query, std::move(builder));
}

ExecuteResponse Transaction::Execute(
    QuerySettings query_settings,
    OperationSettings settings,
    const Query& query,
    PreparedArgsBuilder&& builder
) {
    EnsureActive();

    impl::RequestContext context{table_client_, query, std::move(settings), impl::IsStreaming{false}, &span_};
    auto internal_params = std::move(builder).Build();

    auto exec_query_settings = table_client_.ToExecuteQuerySettings(query_settings);
    impl::ApplyToRequestSettings(exec_query_settings, context.settings, context.deadline, context.span.GetTraceId());

    auto exec_data_query_settings = table_client_.ToExecDataQuerySettings(query_settings);
    impl::ApplyToRequestSettings(
        exec_data_query_settings,
        context.settings,
        context.deadline,
        context.span.GetTraceId()
    );

    // Must go after PrepareExecuteSettings, because an exception from there
    // leaves the transaction active.
    auto error_guard = ErrorGuard();

    using AsyncResultType = std::variant<NYdb::NQuery::TAsyncExecuteQueryResult, NYdb::NTable::TAsyncDataQueryResult>;
    using ResultType = std::variant<NYdb::NQuery::TExecuteQueryResult, NYdb::NTable::TDataQueryResult>;

    auto execute_future = std::visit(
        utils::Overloaded{
            [&internal_params, &query, &exec_query_settings](NYdb::NQuery::TTransaction tx) -> AsyncResultType {
                return tx.GetSession().ExecuteQuery(
                    impl::ToString(query.GetStatementView()),
                    NYdb::NQuery::TTxControl::Tx(tx),
                    std::move(internal_params),
                    exec_query_settings
                );
            },
            [&internal_params, &query, &exec_data_query_settings](NYdb::NTable::TTransaction tx) -> AsyncResultType {
                return tx.GetSession().ExecuteDataQuery(
                    impl::ToString(query.GetStatementView()),
                    NYdb::NTable::TTxControl::Tx(tx),
                    std::move(internal_params),
                    exec_data_query_settings
                );
            }
        },
        ydb_tx_
    );

    auto status = std::visit(
        [this, &context](auto&& execute_future) -> ResultType {
            return impl::GetFutureValueChecked(
                std::forward<decltype(execute_future)>(execute_future),
                "Transaction::Execute",
                table_client_.driver_->GetRetryBudget(),
                context
            );
        },
        std::move(execute_future)
    );

    error_guard.Release();
    return ExecuteResponse(std::move(status));
}

NYdb::TTransactionBase& Transaction::GetNativeTransaction() {
    return std::visit([](auto& tx) -> NYdb::TTransactionBase& { return tx; }, ydb_tx_);
}

}  // namespace ydb

USERVER_NAMESPACE_END
