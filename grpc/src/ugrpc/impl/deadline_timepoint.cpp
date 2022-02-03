#include <userver/ugrpc/impl/deadline_timepoint.hpp>

#include <chrono>
#include <cstdint>

#include <grpc/support/time.h>

::gpr_timespec
grpc::TimePoint<USERVER_NAMESPACE::engine::Deadline>::ToTimePoint(
    USERVER_NAMESPACE::engine::Deadline from) noexcept {
  const auto deadline = from.TimeLeft();
  const auto secs = std::chrono::floor<std::chrono::seconds>(deadline);

  if (secs.count() >= ::gpr_inf_future(::GPR_CLOCK_MONOTONIC).tv_sec) {
    return gpr_inf_future(::GPR_CLOCK_MONOTONIC);
  }
  if (secs.count() < 0) {
    return gpr_inf_past(::GPR_CLOCK_MONOTONIC);
  }

  const auto nsecs =
      std::chrono::duration_cast<std::chrono::nanoseconds>(deadline - secs);
  ::gpr_timespec result;
  result.tv_sec = static_cast<std::int64_t>(secs.count());
  result.tv_nsec = static_cast<std::int32_t>(nsecs.count());
  result.clock_type = ::GPR_TIMESPAN;
  return result;
}
