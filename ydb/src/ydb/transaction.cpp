#include <userver/ydb/transaction.hpp>

#include <userver/formats/json/inline.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/fast_scope_guard.hpp>

#include <userver/ydb/impl/cast.hpp>
#include <userver/ydb/table.hpp>

#include <ydb/impl/config.hpp>
#include <ydb/impl/driver.hpp>
#include <ydb/impl/future.hpp>
#include <ydb/impl/operation_settings.hpp>
#include <ydb/impl/request_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

Transaction::Transaction(TableClient& table_client,
                         NYdb::NTable::TTransaction ydb_tx, std::string name,
                         OperationSettings&& rollback_settings) noexcept
    : table_client_(table_client),
      name_(std::move(name)),
      stats_scope_(impl::StatsScope::TransactionTag{}, *table_client_.stats_,
                   name_),
      span_("ydb_transaction"),
      ydb_tx_(std::move(ydb_tx)),
      rollback_settings_(std::move(rollback_settings)) {
  span_.DetachFromCoroStack();
  span_.AddTag("transaction_name", name_);
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
  impl::RequestContext context{table_client_, kQuery, settings,
                               impl::IsStreaming{false}, &span_};

  if (!name_.empty()) {
    TESTPOINT_CALLBACK("ydb_trx_commit",
                       formats::json::MakeObject("trx_name", name_),
                       [this](const formats::json::Value& data) {
                         if (data["trx_should_fail"].As<bool>()) {
                           LOG_WARNING() << "Doing Rollback instead of commit "
                                            "due to Testpoint response";
                           ydb_tx_.Rollback();
                           throw TransactionForceRollback();
                         }
                       });
  }

  const auto commit_settings =
      impl::PrepareRequestSettings<NYdb::NTable::TCommitTxSettings>(
          settings,
          table_client_.GetDeadline(context.span, context.config_snapshot));

  auto error_guard = ErrorGuard();

  impl::GetFutureValueChecked(ydb_tx_.Commit(commit_settings), "Commit",
                              table_client_.driver_->GetRetryBudget());

  error_guard.Release();
  is_active_ = false;
}

void Transaction::Rollback() {
  EnsureActive();

  static const Query kQuery{"", Query::Name{"Rollback"}};
  auto settings = rollback_settings_;
  impl::RequestContext context{table_client_, kQuery, settings,
                               impl::IsStreaming{false}, &span_};

  const auto rollback_settings =
      impl::PrepareRequestSettings<NYdb::NTable::TRollbackTxSettings>(
          settings,
          table_client_.GetDeadline(context.span, context.config_snapshot));

  [[maybe_unused]] auto error_guard = ErrorGuard();

  impl::GetFutureValueChecked(ydb_tx_.Rollback(rollback_settings), "Rollback",
                              table_client_.driver_->GetRetryBudget());

  // Successful rollback is still a transaction error for logs and stats.
}

PreparedArgsBuilder Transaction::GetBuilder() const {
  return PreparedArgsBuilder(ydb_tx_.GetSession().GetParamsBuilder());
}

void Transaction::EnsureActive() const {
  if (!is_active_) {
    throw InvalidTransactionError();
  }
}

ExecuteResponse Transaction::Execute(OperationSettings settings,
                                     const Query& query,
                                     PreparedArgsBuilder&& builder) {
  return Execute(QuerySettings{}, std::move(settings), query,
                 std::move(builder));
}

ExecuteResponse Transaction::Execute(QuerySettings query_settings,
                                     OperationSettings settings,
                                     const Query& query,
                                     PreparedArgsBuilder&& builder) {
  EnsureActive();

  impl::RequestContext context{table_client_, query, settings,
                               impl::IsStreaming{false}, &span_};
  auto internal_params = std::move(builder).Build();

  auto exec_settings = table_client_.ToExecQuerySettings(query_settings);
  impl::ApplyToRequestSettings(
      exec_settings, settings,
      table_client_.GetDeadline(context.span, context.config_snapshot));

  // Must go after PrepareExecuteSettings, because an exception from there
  // leaves the transaction active.
  auto error_guard = ErrorGuard();

  auto execute_fut = ydb_tx_.GetSession().ExecuteDataQuery(
      impl::ToString(query.Statement()), NYdb::NTable::TTxControl::Tx(ydb_tx_),
      std::move(internal_params), exec_settings);

  auto status = impl::GetFutureValueChecked(
      std::move(execute_fut), "Transaction::Execute",
      table_client_.driver_->GetRetryBudget());

  error_guard.Release();
  return ExecuteResponse(std::move(status));
}

}  // namespace ydb

USERVER_NAMESPACE_END
