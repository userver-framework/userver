#pragma once

#include <userver/utils/statistics/rate_counter.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl::dist_lock {

struct Statistics {
  /// Successes and failures to start coordination session.
  utils::statistics::RateCounter start_session_successes{0};
  utils::statistics::RateCounter start_session_failures{0};

  /// Session disconnections.
  utils::statistics::RateCounter session_stopped{0};

  /// Successful and failed Acquire calls.
  utils::statistics::RateCounter lock_successes{0};
  utils::statistics::RateCounter lock_failures{0};

  /// Task completions without and with an exception.
  utils::statistics::RateCounter task_successes{0};
  utils::statistics::RateCounter task_failures{0};

  /// Counts the number of times where lock was lost, and the task got
  /// cancelled, but it did not do so for at least `cancel_task_time_limit`.
  /// Such a situation can cause "brain split" where multiple hosts are running
  /// `DoWork` at the same time.
  ///
  /// It is a good idea to bind an alert to this metric.
  utils::statistics::RateCounter cancel_task_time_limit_exceeded{0};
};

}  // namespace ydb::impl::dist_lock

USERVER_NAMESPACE_END
