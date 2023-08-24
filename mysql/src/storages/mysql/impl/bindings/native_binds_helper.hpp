#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <userver/formats/json/value.hpp>

#include <userver/storages/mysql/dates.hpp>
#include <userver/storages/mysql/impl/io/decimal_binder.hpp>

#include <storages/mysql/impl/mariadb_include.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::bindings {

struct NativeBindsHelper {
  using MariaDBTimepointResolution = std::chrono::microseconds;

  template <typename T>
  static constexpr enum_field_types GetNativeType();

  template <typename T>
  static constexpr enum_field_types GetNativeType(T&);

  static bool IsFieldNumeric(enum_field_types type);

  static std::size_t NumericFieldWidth(enum_field_types type);

  static std::string_view NativeTypeToString(enum_field_types type);

  static MYSQL_TIME ToNativeTime(std::chrono::system_clock::time_point tp);

  static std::chrono::system_clock::time_point FromNativeTime(
      const MYSQL_TIME& native_time);

  static constexpr std::size_t kOnStackBindsCount = 8;
};

template <typename U>
constexpr enum_field_types NativeBindsHelper::GetNativeType() {
  using T = std::remove_const_t<U>;

  if constexpr (std::is_same_v<std::uint8_t, T> ||
                std::is_same_v<std::int8_t, T>) {
    return MYSQL_TYPE_TINY;
  } else if constexpr (std::is_same_v<std::uint16_t, T> ||
                       std::is_same_v<std::int16_t, T>) {
    return MYSQL_TYPE_SHORT;
  } else if constexpr (std::is_same_v<std::uint32_t, T> ||
                       std::is_same_v<std::int32_t, T>) {
    return MYSQL_TYPE_LONG;
  } else if constexpr (std::is_same_v<std::uint64_t, T> ||
                       std::is_same_v<std::int64_t, T>) {
    return MYSQL_TYPE_LONGLONG;
  } else if constexpr (std::is_same_v<float, T>) {
    return MYSQL_TYPE_FLOAT;
  } else if constexpr (std::is_same_v<double, T>) {
    return MYSQL_TYPE_DOUBLE;
  } else if constexpr (std::is_same_v<std::string, T> ||
                       std::is_same_v<std::string_view, T>) {
    return MYSQL_TYPE_STRING;
  } else if constexpr (std::is_same_v<std::chrono::system_clock::time_point,
                                      T>) {
    return MYSQL_TYPE_TIMESTAMP;
  } else if constexpr (std::is_same_v<Date, T>) {
    return MYSQL_TYPE_DATE;
  } else if constexpr (std::is_same_v<DateTime, T>) {
    return MYSQL_TYPE_DATETIME;
  } else if constexpr (std::is_same_v<formats::json::Value, T>) {
    return MYSQL_TYPE_JSON;
  } else {
    static_assert(!sizeof(T), "This shouldn't fire, fix me");
  }
}

template <typename T>
constexpr enum_field_types NativeBindsHelper::GetNativeType(T&) {
  return GetNativeType<T>();
}

}  // namespace storages::mysql::impl::bindings

USERVER_NAMESPACE_END
