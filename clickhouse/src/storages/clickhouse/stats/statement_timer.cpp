#include "statement_timer.hpp"

#include <exception>

#include <storages/clickhouse/stats/pool_statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::stats {

StatementTimer::StatementTimer(PoolQueryStatistics& stats)
    : stats_{stats},
      exceptions_on_enter_{std::uncaught_exceptions()},
      start_{Now()} {}

StatementTimer::~StatementTimer() {
  ++stats_.total;
  if (std::uncaught_exceptions() != exceptions_on_enter_) {
    ++stats_.error;
  } else {
    stats_.timings.GetCurrentCounter().Account(
        std::chrono::duration_cast<std::chrono::milliseconds>(Now() - start_)
            .count());
  }
}

StatementTimer::Clock::time_point StatementTimer::Now() { return Clock::now(); }

}  // namespace storages::clickhouse::stats

USERVER_NAMESPACE_END
