#pragma once

#include <userver/chaotic/convert/to.hpp>
#include <userver/utils/datetime/timepoint_tz.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::datetime {

TimePointTzFraction Convert(const std::string& str, chaotic::convert::To<TimePointTzFraction>);

std::string Convert(const TimePointTzFraction& tp, chaotic::convert::To<std::string>);

}  // namespace utils::datetime

USERVER_NAMESPACE_END
