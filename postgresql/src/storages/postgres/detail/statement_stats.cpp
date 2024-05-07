#include "statement_stats.hpp"

#include <userver/storages/postgres/detail/connection_ptr.hpp>
#include <userver/storages/postgres/query.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

StatementStats::StatementStats(const Query& query, const ConnectionPtr& conn)
    : query_{query},
      sts_{conn.GetStatementStatsStorage()},
      start_{sts_ != nullptr ? Now() : SteadyClock::time_point{}} {}

void StatementStats::AccountStatementExecution() {
  AccountImpl(StatementStatsStorage::ExecutionResult::kSuccess);
}

void StatementStats::AccountStatementError() {
  AccountImpl(StatementStatsStorage::ExecutionResult::kError);
}

void StatementStats::AccountImpl(
    StatementStatsStorage::ExecutionResult execution_result) {
  if (sts_ == nullptr || !query_.GetName().has_value()) return;

  const auto duration_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(Now() - start_)
          .count();

  sts_->Account(query_.GetName()->GetUnderlying(), duration_ms,
                execution_result);
}

SteadyClock::time_point StatementStats::Now() { return SteadyClock::now(); }

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
