#pragma once

/// @file userver/storages/mysql/dates.hpp

#include <chrono>
#include <cstdint>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

/// @brief a class that represent MySQL DATE type.
class Date final {
 public:
  /// @brief constructs a DATE with year/month/day being zero.
  Date();

  /// @brief constructs a DATE from provided year, month and day.
  Date(std::uint32_t year, std::uint32_t month, std::uint32_t day);

  /// @brief constructs a DATE from provided timepoint.
  explicit Date(std::chrono::system_clock::time_point tp);

  /// @brief converts the DATE to timepoint.
  /// throws if stored date is out of timepoint range.
  std::chrono::system_clock::time_point ToTimePoint() const;

  /// @brief Returns the year part of the date.
  std::uint32_t GetYear() const noexcept;

  /// @brief Returns the month part of the date.
  std::uint32_t GetMonth() const noexcept;

  /// @brief Returns the day part of the date.
  std::uint32_t GetDay() const noexcept;

 private:
  std::uint32_t year_;
  std::uint32_t month_;
  std::uint32_t day_;
};

/// @brief a class that represents MySQL DATETIME type.
class DateTime final {
 public:
  /// @brief constructs a DATETIME with DATE/hour/minute/second/microsecond
  /// being zero.
  DateTime();

  /// @brief constructs a DATETIME from provided DATE, hour, minute, second and
  /// subsecond.
  DateTime(Date date, std::uint32_t hour, std::uint32_t minute,
           std::uint32_t second, std::uint64_t subsecond);

  /// @brief constructs a DATETIME from provided year, month, day, hour, minute,
  /// second and subsecond.
  DateTime(std::uint32_t year, std::uint32_t month, std::uint32_t day,
           std::uint32_t hour, std::uint32_t minute, std::uint32_t second,
           std::uint64_t microsecond);

  /// @brief constructs a DATETIME from provided timepoint.
  explicit DateTime(std::chrono::system_clock::time_point tp);

  /// @brief converts the DATETIME to timepoint.
  /// throws if stored datetime is out of timepoint range.
  std::chrono::system_clock::time_point ToTimePoint() const;

  /// @brief Returns the date part of the datetime.
  const Date& GetDate() const noexcept;

  /// @brief Returns the hour part of the datetime.
  std::uint32_t GetHour() const noexcept;

  /// @brief Returns the minute part of the datetime.
  std::uint32_t GetMinute() const noexcept;

  /// @brief Returns the second part of the datetime.
  std::uint32_t GetSecond() const noexcept;

  /// @brief Returns the subsecond part of the datetime.
  std::uint64_t GetMicrosecond() const noexcept;

 private:
  Date date_;
  std::uint32_t hour_;
  std::uint32_t minute_;
  std::uint32_t second_;
  // TODO : why is this long in MYSQL_TIME?
  std::uint64_t microsecond_;
};

}  // namespace storages::mysql

USERVER_NAMESPACE_END
