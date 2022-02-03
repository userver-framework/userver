#pragma once

#include <grpcpp/support/time.h>

#include <userver/engine/deadline.hpp>

template <>
class grpc::TimePoint<USERVER_NAMESPACE::engine::Deadline> {
 public:
  explicit TimePoint(USERVER_NAMESPACE::engine::Deadline time) noexcept
      : time_(ToTimePoint(time)) {}

  ::gpr_timespec raw_time() const noexcept { return time_; }

 private:
  static ::gpr_timespec ToTimePoint(
      USERVER_NAMESPACE::engine::Deadline from) noexcept;

  ::gpr_timespec time_;
};
