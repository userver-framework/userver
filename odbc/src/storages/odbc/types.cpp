#include <userver/storages/odbc/types.hpp>

#include <stdexcept>
#include <utility>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

namespace {

bool IsLeapYear(std::uint32_t year) noexcept { return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0); }

std::uint32_t DaysInMonth(std::uint32_t year, std::uint32_t month) {
    constexpr std::uint32_t kDays[]{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && IsLeapYear(year)) {
        return 29;
    }
    return kDays[month - 1];
}

}  // namespace

Bytes::Bytes(Container bytes)
    : bytes_{std::move(bytes)}
{}

Bytes::Bytes(std::initializer_list<ValueType> bytes)
    : bytes_{bytes}
{}

const Bytes::Container& Bytes::GetBytes() const noexcept { return bytes_; }

std::size_t Bytes::Size() const noexcept { return bytes_.size(); }

bool Bytes::IsEmpty() const noexcept { return bytes_.empty(); }

Date::Date() noexcept = default;

Date::Date(std::uint32_t year, std::uint32_t month, std::uint32_t day) {
    if (year == 0 || year > 9999) {
        throw std::invalid_argument("ODBC Date year must be in the range 1..9999");
    }
    if (month == 0 || month > 12) {
        throw std::invalid_argument("ODBC Date month must be in the range 1..12");
    }
    if (day == 0 || day > DaysInMonth(year, month)) {
        throw std::invalid_argument("ODBC Date day is outside the selected month");
    }
    year_ = static_cast<std::uint16_t>(year);
    month_ = static_cast<std::uint8_t>(month);
    day_ = static_cast<std::uint8_t>(day);
}

std::uint32_t Date::GetYear() const noexcept { return year_; }

std::uint32_t Date::GetMonth() const noexcept { return month_; }

std::uint32_t Date::GetDay() const noexcept { return day_; }

std::string Date::ToString() const { return fmt::format("{:04}-{:02}-{:02}", year_, month_, day_); }

Time::Time(std::uint32_t hour, std::uint32_t minute, std::uint32_t second) {
    if (hour > 23) {
        throw std::invalid_argument("ODBC Time hour must be in the range 0..23");
    }
    if (minute > 59) {
        throw std::invalid_argument("ODBC Time minute must be in the range 0..59");
    }
    if (second > 59) {
        throw std::invalid_argument("ODBC Time second must be in the range 0..59");
    }
    hour_ = static_cast<std::uint8_t>(hour);
    minute_ = static_cast<std::uint8_t>(minute);
    second_ = static_cast<std::uint8_t>(second);
}

std::uint32_t Time::GetHour() const noexcept { return hour_; }

std::uint32_t Time::GetMinute() const noexcept { return minute_; }

std::uint32_t Time::GetSecond() const noexcept { return second_; }

std::string Time::ToString() const { return fmt::format("{:02}:{:02}:{:02}", hour_, minute_, second_); }

Timestamp::Timestamp(Date date, Time time, std::uint32_t fraction_nanoseconds)
    : date_{date},
      time_{time},
      fraction_nanoseconds_{fraction_nanoseconds}
{
    if (fraction_nanoseconds > 999'999'999) {
        throw std::invalid_argument("ODBC Timestamp fraction must be in the range 0..999999999 nanoseconds");
    }
}

Timestamp::Timestamp(
    std::uint32_t year,
    std::uint32_t month,
    std::uint32_t day,
    std::uint32_t hour,
    std::uint32_t minute,
    std::uint32_t second,
    std::uint32_t fraction_nanoseconds
)
    : Timestamp{Date{year, month, day}, Time{hour, minute, second}, fraction_nanoseconds}
{}

const Date& Timestamp::GetDate() const noexcept { return date_; }

const Time& Timestamp::GetTime() const noexcept { return time_; }

std::uint32_t Timestamp::GetFractionNanoseconds() const noexcept { return fraction_nanoseconds_; }

std::string Timestamp::ToString() const {
    if (fraction_nanoseconds_ == 0) {
        return fmt::format("{} {}", date_.ToString(), time_.ToString());
    }
    return fmt::format("{} {}.{:09}", date_.ToString(), time_.ToString(), fraction_nanoseconds_);
}

}  // namespace storages::odbc

USERVER_NAMESPACE_END
