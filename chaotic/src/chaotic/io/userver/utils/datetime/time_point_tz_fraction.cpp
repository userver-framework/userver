#include <userver/chaotic/io/userver/utils/datetime/time_point_tz.hpp>

#include <cctz/time_zone.h>
#include <fmt/format.h>

#include <userver/utils/assert.hpp>
#include <userver/utils/datetime/from_string_saturating.hpp>
#include <userver/utils/datetime_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::datetime {

namespace {
constexpr std::string_view kZeroTimePoint = "1970-01-01T00:00:00";
}

TimePointTzFraction Convert(const std::string& str, chaotic::convert::To<TimePointTzFraction>) {
    auto s = str;
    auto tp = utils::datetime::FromStringSaturating(s, utils::datetime::kRfc3339Format);

    UINVARIANT(s.size() >= kZeroTimePoint.size(), "Invalid datetime");
    memcpy(s.data(), kZeroTimePoint.data(), kZeroTimePoint.length());
    auto tp_tz = utils::datetime::UtcStringtime(s, utils::datetime::kFractionFormat);
    return TimePointTzFraction{tp, -std::chrono::duration_cast<std::chrono::seconds>(tp_tz.time_since_epoch())};
}

std::string Convert(const TimePointTzFraction& tp, chaotic::convert::To<std::string>) {
    auto offset = tp.GetTzOffset();
    return cctz::format(utils::datetime::kFractionFormat, tp.GetTimePoint(), cctz::fixed_time_zone(offset));
}

}  // namespace utils::datetime

USERVER_NAMESPACE_END
