#pragma once

#include <userver/chaotic/convert/to.hpp>
#include <userver/utils/datetime/timepoint_tz.hpp>

namespace my {

struct CustomDateTime {
  USERVER_NAMESPACE::utils::datetime::TimePointTz time_point;

  CustomDateTime(USERVER_NAMESPACE::utils::datetime::TimePointTz time_point)
      : time_point(time_point) {}
};

inline bool operator==(const CustomDateTime& lhs, const CustomDateTime& rhs) {
  return lhs.time_point == rhs.time_point;
}

inline USERVER_NAMESPACE::utils::datetime::TimePointTz Convert(
    const CustomDateTime& lhs,
    USERVER_NAMESPACE::chaotic::convert::To<
        USERVER_NAMESPACE::utils::datetime::TimePointTz>) {
  return lhs.time_point;
}

}  // namespace my
