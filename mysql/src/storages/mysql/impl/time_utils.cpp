#include <storages/mysql/impl/time_utils.hpp>

#include <userver/utils/datetime/cpp_20_calendar.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

namespace date = utils::datetime::date;

MYSQL_TIME TimeUtils::ToNativeTime(std::chrono::system_clock::time_point tp) {
  // https://stackoverflow.com/a/15958113/10508079
  const auto dp = date::floor<date::days>(tp);
  const auto ymd = date::year_month_day{dp};
  const auto time = date::make_time(
      std::chrono::duration_cast<std::chrono::microseconds>(tp - dp));

  MYSQL_TIME native_time{};
  native_time.year = ymd.year().operator int();
  native_time.month = ymd.month().operator unsigned int();
  native_time.day = ymd.day().operator unsigned int();
  native_time.hour = time.hours().count();
  native_time.minute = time.minutes().count();
  native_time.second = time.seconds().count();
  native_time.second_part = time.subseconds().count();

  return native_time;
}

std::chrono::system_clock::time_point TimeUtils::FromNativeTime(
    const MYSQL_TIME& native_time) {
  // https://stackoverflow.com/a/15958113/10508079
  const date::year year(native_time.year);
  const date::month month{native_time.month};
  const date::day day{native_time.day};
  const std::chrono::hours hour{native_time.hour};
  const std::chrono::minutes minute{native_time.minute};
  const std::chrono::seconds second{native_time.second};
  const std::chrono::microseconds microsecond{native_time.second_part};

  return date::sys_days{year / month / day} + hour + minute + second +
         microsecond;
}

Date TimeUtils::DateFromChrono(std::chrono::system_clock::time_point tp) {
  const auto ymd = date::year_month_day{date::floor<date::days>(tp)};
  // TODO : is this static_cast ok?
  return Date{static_cast<std::uint32_t>(ymd.year().operator int()),
              ymd.month().operator unsigned int(),
              ymd.day().operator unsigned int()};
}

std::chrono::system_clock::time_point TimeUtils::DateToChrono(
    const Date& date) {
  const date::year year(date.GetYear());
  const date::month month{date.GetMonth()};
  const date::day day{date.GetDay()};

  return date::sys_days{year / month / day};
}

DateTime TimeUtils::DateTimeFromChrono(
    std::chrono::system_clock::time_point tp) {
  const auto dp = date::floor<date::days>(tp);
  const auto ymd = date::year_month_day{dp};
  const auto time = date::make_time(
      std::chrono::duration_cast<std::chrono::microseconds>(tp - dp));

  // TODO : signs in all these casts?
  return {static_cast<uint32_t>(ymd.year().operator int()),
          ymd.month().operator unsigned int(),
          ymd.day().operator unsigned int(),
          static_cast<uint32_t>(time.hours().count()),
          static_cast<uint32_t>(time.minutes().count()),
          static_cast<uint32_t>(time.seconds().count()),
          static_cast<uint64_t>(time.subseconds().count())};
}

std::chrono::system_clock::time_point TimeUtils::DateTimeToChrono(
    const DateTime& datetime) {
  const date::year year(datetime.GetDate().GetYear());
  const date::month month{datetime.GetDate().GetMonth()};
  const date::day day{datetime.GetDate().GetDay()};
  const std::chrono::hours hour{datetime.GetHour()};
  const std::chrono::minutes minute{datetime.GetMinute()};
  const std::chrono::seconds second{datetime.GetSecond()};
  const std::chrono::microseconds microsecond{datetime.GetMicrosecond()};

  return date::sys_days{year / month / day} + hour + minute + second +
         microsecond;
}

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
