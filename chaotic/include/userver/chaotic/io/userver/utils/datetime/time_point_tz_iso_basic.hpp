#pragma once

#include <userver/chaotic/convert/to.hpp>
#include <userver/utils/datetime/timepoint_tz.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::datetime {

TimePointTzIsoBasic Convert(const std::string& str,
                            chaotic::convert::To<TimePointTzIsoBasic>);

std::string Convert(const TimePointTzIsoBasic& tp,
                    chaotic::convert::To<std::string>);

}  // namespace utils::datetime

USERVER_NAMESPACE_END
