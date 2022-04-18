#pragma once

#include <chrono>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::stats {

struct PoolQueryStatistics;

class StatementTimer final {
 public:
  StatementTimer(PoolQueryStatistics& stats);
  ~StatementTimer();

 private:
  using Clock = std::chrono::steady_clock;

  static Clock::time_point Now();

  PoolQueryStatistics& stats_;
  const int exceptions_on_enter_;
  Clock::time_point start_;
};

}  // namespace storages::clickhouse::stats

USERVER_NAMESPACE_END
