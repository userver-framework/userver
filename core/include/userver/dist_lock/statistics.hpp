#pragma once

#include <cstddef>

#include <userver/utils/statistics/relaxed_counter.hpp>

namespace dist_lock {

struct Statistics {
  utils::statistics::RelaxedCounter<size_t> successes{0};
  utils::statistics::RelaxedCounter<size_t> failures{0};
  utils::statistics::RelaxedCounter<size_t> watchdog_triggers{0};
  utils::statistics::RelaxedCounter<size_t> brain_splits{0};
};

}  // namespace dist_lock
