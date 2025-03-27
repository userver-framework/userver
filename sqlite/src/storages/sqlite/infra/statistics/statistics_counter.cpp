#include <userver/storages/sqlite/infra/statistics/statistics_counter.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra::statistics {

CountExecute::CountExecute(PoolQueryStatistics& stats)
    : stats_{stats},
      exceptions_on_enter_{std::uncaught_exceptions()},
      exec_begin_time{utils::datetime::SteadyClock::now()} {}

CountExecute::~CountExecute() {
  ++stats_.total;
  auto now = utils::datetime::SteadyClock::now();
  if (std::uncaught_exceptions() != exceptions_on_enter_) {
    ++stats_.error;
  } else {
    stats_.timings.GetCurrentCounter().Account(
        std::chrono::duration_cast<std::chrono::milliseconds>(now -
                                                              exec_begin_time)
            .count());
  }
}

}  // namespace storages::sqlite::infra::statistics

USERVER_NAMESPACE_END
