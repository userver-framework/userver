#pragma once

/// @file userver/utils/datetime/timepoint_tz.hpp
/// @brief Timepoint with timezone
/// @ingroup userver_universal

#include <chrono>

#include <userver/logging/log_helper.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::datetime {

class TimePointTzBase {
 public:
  using TimePoint = std::chrono::system_clock::time_point;

  constexpr TimePointTzBase() = default;
  constexpr explicit TimePointTzBase(TimePoint tp) : tp_(tp) {}
  constexpr TimePointTzBase(TimePoint tp, std::chrono::seconds tz_offset)
      : tp_(tp), tz_offset_(tz_offset) {}
  TimePointTzBase(const TimePointTzBase& other);
  TimePointTzBase(TimePointTzBase&& other) noexcept;

  TimePointTzBase& operator=(const TimePointTzBase& other);
  TimePointTzBase& operator=(TimePointTzBase&& other) noexcept;

  // Deliberately no 'explicit' to be compatible with legacy util/swaggen's
  // plain time_point types
  operator TimePoint() const;

  /// Get timezone in seconds (may be negative)
  std::chrono::seconds GetTzOffset() const;

  /// Get std's time_point
  TimePoint GetTimePoint() const;

  bool operator==(const TimePointTzBase& other) const;

  bool operator<(const TimePointTzBase& other) const;

  bool operator>(const TimePointTzBase& other) const;

  bool operator<=(const TimePointTzBase& other) const;

  bool operator>=(const TimePointTzBase& other) const;

 private:
  TimePoint tp_{};
  std::chrono::seconds tz_offset_{};
};

bool operator<(const TimePointTzBase::TimePoint& lhs,
               const TimePointTzBase& rhs);

bool operator>(const TimePointTzBase::TimePoint& lhs,
               const TimePointTzBase& rhs);

bool operator<=(const TimePointTzBase::TimePoint& lhs,
                const TimePointTzBase& rhs);

bool operator>=(const TimePointTzBase::TimePoint& lhs,
                const TimePointTzBase& rhs);

/// Timepoint with timezone parsed in kRfc3339Format
class TimePointTz final : public TimePointTzBase {
  using TimePointTzBase::TimePointTzBase;
};

logging::LogHelper& operator<<(logging::LogHelper& os, const TimePointTz& v);

/// Timepoint with timezone parsed in kDefaultFormat
class TimePointTzIsoBasic final : public TimePointTzBase {
  using TimePointTzBase::TimePointTzBase;
};

logging::LogHelper& operator<<(logging::LogHelper& os,
                               const TimePointTzIsoBasic& v);

}  // namespace utils::datetime

USERVER_NAMESPACE_END
