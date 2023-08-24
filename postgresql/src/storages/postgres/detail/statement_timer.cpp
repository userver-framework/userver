#include "statement_timer.hpp"

#include <storages/postgres/detail/statement_timings_storage.hpp>
#include <userver/storages/postgres/detail/connection_ptr.hpp>
#include <userver/storages/postgres/query.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

StatementTimer::StatementTimer(const Query& query, const ConnectionPtr& conn)
    : query_{query},
      sts_{conn.GetStatementTimingsStorage()},
      start_{sts_ != nullptr ? Now() : SteadyClock::time_point{}} {}

void StatementTimer::Account() {
  if (sts_ == nullptr || !query_.GetName().has_value()) return;

  const auto duration_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(Now() - start_)
          .count();

  sts_->Account(query_.GetName()->GetUnderlying(), duration_ms);
}

SteadyClock::time_point StatementTimer::Now() { return SteadyClock::now(); }

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
