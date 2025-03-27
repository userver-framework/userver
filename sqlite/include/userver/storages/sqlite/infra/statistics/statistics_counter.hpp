#pragma once

#include <userver/storages/sqlite/infra/statistics/statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra::statistics {

class CountExecute {
 public:
  CountExecute(PoolQueryStatistics& stats);

  ~CountExecute();

 private:
  PoolQueryStatistics& stats_;
  const int exceptions_on_enter_;
  utils::datetime::SteadyClock::time_point exec_begin_time;
};

}  // namespace storages::sqlite::infra::statistics

USERVER_NAMESPACE_END
