#pragma once

#include <cstddef>

#include <userver/utils/statistics/relaxed_counter.hpp>

USERVER_NAMESPACE_BEGIN

namespace dist_lock {

struct Statistics {
  utils::statistics::RelaxedCounter<size_t> lock_successes{0};
  utils::statistics::RelaxedCounter<size_t> lock_failures{0};
  utils::statistics::RelaxedCounter<size_t> watchdog_triggers{0};
  utils::statistics::RelaxedCounter<size_t> brain_splits{0};
  utils::statistics::RelaxedCounter<size_t> task_failures{0};
};

}  // namespace dist_lock

USERVER_NAMESPACE_END
