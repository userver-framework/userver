#include <userver/utils/datetime/timepoint_tz.hpp>

#include <tuple>

#include <userver/utils/datetime.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::datetime {

TimePointTzBase::TimePointTzBase(const TimePointTzBase& other) = default;

TimePointTzBase::TimePointTzBase(TimePointTzBase&& other) noexcept = default;

TimePointTzBase& TimePointTzBase::operator=(const TimePointTzBase& other) =
    default;

TimePointTzBase& TimePointTzBase::operator=(TimePointTzBase&& other) noexcept =
    default;

TimePointTzBase::operator TimePoint() const { return tp_; }

std::chrono::seconds TimePointTzBase::GetTzOffset() const { return tz_offset_; }

TimePointTzBase::TimePoint TimePointTzBase::GetTimePoint() const { return tp_; }

bool TimePointTzBase::operator==(const TimePointTzBase& other) const {
  return tp_ == other.tp_ && tz_offset_ == other.tz_offset_;
}

bool TimePointTzBase::operator<(const TimePointTzBase& other) const {
  return tp_ < other.tp_;
}

bool TimePointTzBase::operator>(const TimePointTzBase& other) const {
  return tp_ > other.tp_;
}

bool TimePointTzBase::operator<=(const TimePointTzBase& other) const {
  return tp_ <= other.tp_;
}

bool TimePointTzBase::operator>=(const TimePointTzBase& other) const {
  return tp_ >= other.tp_;
}

bool operator<(const TimePointTzBase::TimePoint& lhs,
               const TimePointTzBase& rhs) {
  return lhs < rhs.GetTimePoint();
}

bool operator>(const TimePointTzBase::TimePoint& lhs,
               const TimePointTzBase& rhs) {
  return lhs > rhs.GetTimePoint();
}

bool operator<=(const TimePointTzBase::TimePoint& lhs,
                const TimePointTzBase& rhs) {
  return lhs <= rhs.GetTimePoint();
}

bool operator>=(const TimePointTzBase::TimePoint& lhs,
                const TimePointTzBase& rhs) {
  return lhs >= rhs.GetTimePoint();
}

logging::LogHelper& operator<<(logging::LogHelper& os, const TimePointTz& v) {
  return os << v.GetTimePoint();
}

logging::LogHelper& operator<<(logging::LogHelper& os,
                               const TimePointTzIsoBasic& v) {
  return os << v.GetTimePoint();
}

}  // namespace utils::datetime

USERVER_NAMESPACE_END
