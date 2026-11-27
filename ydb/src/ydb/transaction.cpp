#include <userver/ydb/transaction.hpp>

#include <algorithm>
#include <chrono>
#include <exception>
#include <utility>

#include <userver/engine/task/cancel.hpp>
#include <userver/formats/json/inline.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/assert.hpp>
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
namespace {

template <typename Cleanup>
void WaitForExecuteWithLateSuccessCleanup(const NYdb::NQuery::TAsyncExecuteQueryResult& future, Cleanup cleanup) {
    try {
        impl::WaitForFuture(future);
    } catch (const OperationCancelledError&) {
        future.Subscribe([cleanup = std::move(cleanup)](const auto& ready) mutable {
            try {
                if (ready.GetValue().IsSuccess()) {
                    cleanup();
                }
            } catch (const std::exception& ex) {
                LOG_WARNING() << "Late transaction cleanup failed: " << ex;
            }
        });
        throw;
    }
}

std::chrono::milliseconds GetRollbackTimeout(std::chrono::milliseconds timeout) {
    constexpr std::chrono::milliseconds kMaxRollbackTimeout{std::chrono::seconds{1}};
    return timeout > std::chrono::milliseconds::zero() ? std::min(timeout, kMaxRollbackTimeout) : kMaxRollbackTimeout;
}

}  // namespace

namespace {

struct TxQueryContext {
    NYdb::NQuery::TSession session;
    NYdb::NQuery::TTxControl control;
};

}  // namespace

TxActor::TxActor(
    TableClient& table_client,
    NYdb::NQuery::TSession session,
    NYdb::NQuery::TTxSettings&& tx_settings,
    engine::Deadline deadline,
    std::uint32_t attempt
)
    : table_client_(table_client),
      deadline_(deadline),
      attempt_(attempt),
      tx_settings_(std::move(tx_settings)),
      session_or_tx_(std::move(session))
{}

ExecuteResponse TxActor::Execute(ExecuteSettings settings, const Query& query, PreparedArgsBuilder&& builder) {
    auto context =
        table_client_.MakeRequestContext(query, std::move(settings), impl::IsStreaming{false}, nullptr, deadline_);
    context.span.AddTag("attempt", attempt_);

    auto internal_params = std::move(builder).Build();

    auto exec_settings = impl::PrepareRequestSettings<
        NYdb::NQuery::TExecuteQuerySettings,
        ExecuteSettings>(context.settings, context.deadline, context.span.GetTraceId());

    auto query_context = std::visit(
        utils::Overloaded{
            [this](const NYdb::NQuery::TSession& session) {
                return TxQueryContext{
                    .session = session,
                    .control = NYdb::NQuery::TTxControl::BeginTx(tx_settings_),
                };
            },
            [](const NYdb::NQuery::TTransaction& transaction) {
                return TxQueryContext{
                    .session = transaction.GetSession(),
                    .control = NYdb::NQuery::TTxControl::Tx(transaction),
                };
            },
        },
        session_or_tx_
    );
    auto execute_fut = query_context.session.ExecuteQuery(
        impl::ToString(query.GetStatementView()),
        query_context.control,
        std::move(internal_params),
        exec_settings
    );

    auto status = impl::GetFutureValueChecked(std::move(execute_fut), "TxActor::Execute", context);
    if (std::holds_alternative<NYdb::NQuery::TSession>(session_or_tx_)) {
        auto transaction = status.GetTransaction();
        UINVARIANT(transaction.has_value(), "YDB did not return the transaction started by the query");
        session_or_tx_ = std::move(transaction).value();
    }

    return ExecuteResponse(std::move(status));
}

template <TxAction Action>
void TxActor::FinishTx(const RequestSettings& settings) {
    auto* transaction_ptr = std::get_if<NYdb::NQuery::TTransaction>(&session_or_tx_);
    if (!transaction_ptr) {
        if (deadline_.IsReached()) {
            throw DeadlineExceededError("Transaction deadline exceeded before completion");
        }
        return;
    }
    auto& transaction = *transaction_ptr;
    constexpr std::string_view action_name = Action == TxAction::kCommit ? "Commit" : "Rollback";

    auto context = table_client_.MakeRequestContext(
        Query{"", Query::Name{action_name}},
        RequestSettings{settings},
        impl::IsStreaming{false},
        nullptr,
        deadline_
    );
    context.span.AddTag("attempt", attempt_);

    using SettingsType = std::conditional_t<
        Action == TxAction::kCommit,
        NYdb::NQuery::TCommitTxSettings,
        NYdb::NQuery::TRollbackTxSettings>;

    const auto ydb_settings = impl::PrepareRequestSettings<
        SettingsType>(context.settings, context.deadline, context.span.GetTraceId());

    if constexpr (Action == TxAction::kCommit) {
        auto future = transaction.Commit(ydb_settings);
        impl::GetFutureValueChecked(std::move(future), action_name, context);
    } else {
        auto future = transaction.Rollback(ydb_settings);
        impl::GetFutureValueChecked(std::move(future), action_name, context);
    }
}

template void TxActor::FinishTx<TxAction::kCommit>(const RequestSettings& settings);
template void TxActor::FinishTx<TxAction::kRollback>(const RequestSettings& settings);

PreparedArgsBuilder TxActor::GetBuilder() const { return table_client_.GetBuilder(); }

Transaction::Transaction(
    TableClient& table_client,
    NYdb::NQuery::TTransaction ydb_tx,
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

Transaction::Transaction(Transaction&& other) noexcept
    : table_client_(other.table_client_),
      name_(std::move(other.name_)),
      stats_scope_(std::move(other.stats_scope_)),
      span_(std::move(other.span_)),
      ydb_tx_(std::move(other.ydb_tx_)),
      rollback_settings_(std::move(other.rollback_settings_)),
      is_active_(std::exchange(other.is_active_, false)),
      trx_lock_(std::move(other.trx_lock_)) {}

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
        // Rollback blocks cancellation while cleaning up. Check the request itself,
        // since ShouldCancel() is false under TaskCancellationBlocker.
        if (engine::current_task::IsCancelRequested()) {
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
    auto context = table_client_.MakeRequestContext(kQuery, std::move(settings), impl::IsStreaming{false}, &span_);

    if (!name_.empty()) {
        TESTPOINT_CALLBACK(
            "ydb_trx_commit",
            formats::json::MakeObject("trx_name", name_),
            [this](const formats::json::Value& data) {
                if (data["trx_should_fail"].As<bool>()) {
                    LOG_WARNING()
                        << "Doing Rollback instead of commit "
                           "due to Testpoint response";
                    Rollback();
                    throw TransactionForceRollback();
                }
            }
        );
    }

    const auto commit_settings = impl::PrepareRequestSettings<
        NYdb::NQuery::TCommitTxSettings>(context.settings, context.deadline);

    [[maybe_unused]] auto error_guard = ErrorGuard();

    impl::GetFutureValueChecked(ydb_tx_.Commit(commit_settings), "Commit", table_client_.GetRetryBudget(), context);

    error_guard.Release();
    is_active_ = false;
    trx_lock_.Unlock();
}

void Transaction::Rollback() {
    EnsureActive();

    const engine::TaskCancellationBlocker cancellation_blocker;
    const server::request::DeadlinePropagationBlocker deadline_blocker;
    const auto deadline = engine::Deadline::FromDuration(GetRollbackTimeout(rollback_settings_.client_timeout_ms));
    static const Query kQuery{"", Query::Name{"Rollback"}};
    auto settings = rollback_settings_;
    auto context =
        table_client_.MakeRequestContext(kQuery, std::move(settings), impl::IsStreaming{false}, &span_, deadline);

    const auto rollback_settings = impl::PrepareRequestSettings<
        NYdb::NQuery::TRollbackTxSettings>(context.settings, context.deadline);

    [[maybe_unused]] auto error_guard = ErrorGuard();

    auto future = ydb_tx_.Rollback(rollback_settings);
    impl::WaitForFuture(future, deadline);
    impl::GetFutureValueChecked(std::move(future), "Rollback", table_client_.GetRetryBudget(), context);

    trx_lock_.Unlock();

    // Successful rollback is still a transaction error for logs and stats.
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

    auto context = table_client_.MakeRequestContext(query, std::move(settings), impl::IsStreaming{false}, &span_);
    auto internal_params = std::move(builder).Build();

    auto exec_query_settings = table_client_.ToExecuteQuerySettings(query_settings);
    impl::ApplyToRequestSettings(exec_query_settings, context.settings, context.deadline, context.span.GetTraceId());

    // Must go after PrepareExecuteSettings, because an exception from there
    // leaves the transaction active.
    auto error_guard = ErrorGuard();

    auto execute_future = ydb_tx_.GetSession().ExecuteQuery(
        impl::ToString(query.GetStatementView()),
        NYdb::NQuery::TTxControl::Tx(ydb_tx_),
        std::move(internal_params),
        exec_query_settings
    );

    // SDK callbacks run detached and may outlive the cancelled task and this Transaction.
    // Capture the SDK transaction by value so cleanup remains valid after their destruction.
    WaitForExecuteWithLateSuccessCleanup(
        execute_future,
        [tx = ydb_tx_, timeout = GetRollbackTimeout(rollback_settings_.client_timeout_ms)]() mutable {
            tx.Rollback(NYdb::NQuery::TRollbackTxSettings{}.ClientTimeout(timeout)).Subscribe([](const auto& future) {
                try {
                    const auto& status = future.GetValue();
                    if (!status.IsSuccess()) {
                        LOG_WARNING() << "Late ROLLBACK failed: " << status.GetIssues().ToString();
                    }
                } catch (const std::exception& ex) {
                    LOG_WARNING() << "Late ROLLBACK failed: " << ex;
                }
            });
        }
    );
    auto status = impl::GetFutureValueChecked(
        std::move(execute_future),
        "Transaction::Execute",
        table_client_.GetRetryBudget(),
        context
    );

    error_guard.Release();
    return ExecuteResponse(std::move(status));
}

NYdb::TTransactionBase& Transaction::GetNativeTransaction() { return ydb_tx_; }

}  // namespace ydb

USERVER_NAMESPACE_END
