#pragma once

#include <chrono>

#include <storages/mysql/impl/mariadb_include.hpp>

#include <userver/storages/mysql/dates.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

class TimeUtils final {
 public:
  static MYSQL_TIME ToNativeTime(std::chrono::system_clock::time_point tp);
  static std::chrono::system_clock::time_point FromNativeTime(
      const MYSQL_TIME& native_time);

  static Date DateFromChrono(std::chrono::system_clock::time_point tp);
  static std::chrono::system_clock::time_point DateToChrono(const Date& date);

  static DateTime DateTimeFromChrono(std::chrono::system_clock::time_point tp);
  static std::chrono::system_clock::time_point DateTimeToChrono(
      const DateTime& datetime);
};

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
