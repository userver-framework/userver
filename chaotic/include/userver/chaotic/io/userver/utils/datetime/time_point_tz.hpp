#pragma once

#include <userver/chaotic/convert/to.hpp>
#include <userver/utils/datetime/timepoint_tz.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::datetime {

TimePointTz Convert(const std::string& str, chaotic::convert::To<TimePointTz>);

std::string Convert(const TimePointTz& tp, chaotic::convert::To<std::string>);

}  // namespace utils::datetime

USERVER_NAMESPACE_END
