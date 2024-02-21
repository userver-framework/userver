#pragma once

#include <grpcpp/support/time.h>

#include <userver/engine/deadline.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

// Converts a engine::Deadline::Duration to a gpr_timespec.
::gpr_timespec ToGprTimePoint(engine::Deadline::Duration from) noexcept;

// Converts gpr_timespec to a engine::Deadline::Duration
// returning engine::Deadline::Duration::max() on unreachable deadline.
engine::Deadline::Duration ExtractDeadlineDuration(::gpr_timespec deadline);

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END

template <>
class grpc::TimePoint<USERVER_NAMESPACE::engine::Deadline> {
 public:
  explicit TimePoint(USERVER_NAMESPACE::engine::Deadline time) noexcept
      : time_(USERVER_NAMESPACE::ugrpc::impl::ToGprTimePoint(time.TimeLeft())) {
  }

  ::gpr_timespec raw_time() const noexcept { return time_; }

 private:
  ::gpr_timespec time_;
};
