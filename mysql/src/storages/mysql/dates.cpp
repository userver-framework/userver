#include <userver/storages/mysql/dates.hpp>

#include <storages/mysql/impl/time_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

Date::Date() : Date{0, 0, 0} {}

Date::Date(std::uint32_t year, std::uint32_t month, std::uint32_t day)
    : year_{year}, month_{month}, day_{day} {}

Date::Date(std::chrono::system_clock::time_point tp)
    : Date{impl::TimeUtils::DateFromChrono(tp)} {}

std::chrono::system_clock::time_point Date::ToTimePoint() const {
  return impl::TimeUtils::DateToChrono(*this);
}

std::uint32_t Date::GetYear() const noexcept { return year_; }

std::uint32_t Date::GetMonth() const noexcept { return month_; }

std::uint32_t Date::GetDay() const noexcept { return day_; }

bool Date::operator==(const Date& other) const noexcept {
  return year_ == other.year_ && month_ == other.month_ && day_ == other.day_;
}

DateTime::DateTime() : DateTime{Date{}, 0, 0, 0, 0} {}

DateTime::DateTime(Date date, std::uint32_t hour, std::uint32_t minute,
                   std::uint32_t second, std::uint64_t microsecond)
    : date_{date},
      hour_{hour},
      minute_{minute},
      second_{second},
      microsecond_{microsecond} {}

DateTime::DateTime(std::uint32_t year, std::uint32_t month, std::uint32_t day,
                   std::uint32_t hour, std::uint32_t minute,
                   std::uint32_t second, std::uint64_t microsecond)
    : DateTime(Date{year, month, day}, hour, minute, second, microsecond) {}

DateTime::DateTime(std::chrono::system_clock::time_point tp)
    : DateTime{impl::TimeUtils::DateTimeFromChrono(tp)} {}

std::chrono::system_clock::time_point DateTime::ToTimePoint() const {
  return impl::TimeUtils::DateTimeToChrono(*this);
}

const Date& DateTime::GetDate() const noexcept { return date_; }

std::uint32_t DateTime::GetHour() const noexcept { return hour_; }

std::uint32_t DateTime::GetMinute() const noexcept { return minute_; }

std::uint32_t DateTime::GetSecond() const noexcept { return second_; }

std::uint64_t DateTime::GetMicrosecond() const noexcept { return microsecond_; }

bool DateTime::operator==(const DateTime& other) const noexcept {
  return date_ == other.date_ && hour_ == other.hour_ &&
         minute_ == other.minute_ && second_ == other.second_ &&
         microsecond_ == other.microsecond_;
}

}  // namespace storages::mysql

USERVER_NAMESPACE_END
