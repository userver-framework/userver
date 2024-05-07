#pragma once

#include <storages/postgres/detail/statement_stats_storage.hpp>
#include <userver/storages/postgres/detail/time_types.hpp>
#include <userver/storages/postgres/query.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

class ConnectionPtr;
class StatementStatsStorage;

class StatementStats final {
 public:
  StatementStats(const Query& query, const ConnectionPtr& conn);

  void AccountStatementExecution();
  void AccountStatementError();

 private:
  void AccountImpl(StatementStatsStorage::ExecutionResult execution_result);

  static SteadyClock::time_point Now();

 private:
  const Query& query_;
  const StatementStatsStorage* sts_;

  const SteadyClock::time_point start_;
};

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
