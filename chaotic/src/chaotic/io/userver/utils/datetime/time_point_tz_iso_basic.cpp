#include <userver/chaotic/io/userver/utils/datetime/time_point_tz_iso_basic.hpp>

#include <cctz/time_zone.h>
#include <fmt/format.h>

#include <userver/utils/assert.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/datetime/from_string_saturating.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::datetime {

namespace {
constexpr std::string_view kZeroTimePoint = "1970-01-01T00:00:00";
}

TimePointTzIsoBasic Convert(const std::string& str,
                            chaotic::convert::To<TimePointTzIsoBasic>) {
  auto s = str;
  auto tp =
      utils::datetime::FromStringSaturating(s, utils::datetime::kDefaultFormat);

  UINVARIANT(s.size() >= kZeroTimePoint.size(), "Invalid datetime");
  memcpy(s.data(), kZeroTimePoint.data(), kZeroTimePoint.length());
  auto tp_tz = utils::datetime::Stringtime(s, utils::datetime::kDefaultTimezone,
                                           utils::datetime::kDefaultFormat);
  return TimePointTzIsoBasic{tp,
                             -std::chrono::duration_cast<std::chrono::seconds>(
                                 tp_tz.time_since_epoch())};
}

std::string Convert(const TimePointTzIsoBasic& tp,
                    chaotic::convert::To<std::string>) {
  auto offset = tp.GetTzOffset();
  return cctz::format(utils::datetime::kDefaultFormat, tp.GetTimePoint(),
                      cctz::fixed_time_zone(offset));
}

}  // namespace utils::datetime

USERVER_NAMESPACE_END
