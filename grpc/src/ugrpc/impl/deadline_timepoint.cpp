#include <userver/ugrpc/impl/deadline_timepoint.hpp>

#include <chrono>
#include <cstdint>

#include <grpc/support/time.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

::gpr_timespec ToGprTimePoint(engine::Deadline::Duration deadline) noexcept {
  const auto secs = std::chrono::floor<std::chrono::seconds>(deadline);

  if (secs.count() >= ::gpr_inf_future(::GPR_CLOCK_MONOTONIC).tv_sec) {
    return gpr_inf_future(::GPR_CLOCK_MONOTONIC);
  }
  if (secs.count() < 0) {
    return gpr_inf_past(::GPR_CLOCK_MONOTONIC);
  }

  const auto nsecs =
      std::chrono::duration_cast<std::chrono::nanoseconds>(deadline - secs);
  ::gpr_timespec time_span;
  time_span.tv_sec = static_cast<std::int64_t>(secs.count());
  time_span.tv_nsec = static_cast<std::int32_t>(nsecs.count());
  time_span.clock_type = ::GPR_TIMESPAN;

  return gpr_convert_clock_type(time_span, ::GPR_CLOCK_MONOTONIC);
}

engine::Deadline::Duration ExtractDeadlineDuration(::gpr_timespec deadline) {
  using Duration = engine::Deadline::Duration;
  if (gpr_time_cmp(deadline, gpr_inf_future(deadline.clock_type)) == 0) {
    return Duration::max();
  }

  // May call some now() function and lose precision if not ::GPR_TIMESPAN
  const auto time_span = gpr_convert_clock_type(deadline, ::GPR_TIMESPAN);
  UASSERT(time_span.clock_type == ::GPR_TIMESPAN);

  const auto duration = Duration{std::chrono::seconds{time_span.tv_sec}} +
                        Duration{std::chrono::nanoseconds{time_span.tv_nsec}};

  // In some versions of gRPC, absence of deadline represented as negative
  if (duration < Duration{0}) {
    return Duration::max();
  }

  if (duration >= std::chrono::hours{365 * 24}) {
    return Duration::max();
  }

  return duration;
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
