#include <storages/mysql/impl/bindings/binds_storage_interface.hpp>

#include <date.h>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::bindings {

namespace {

constexpr std::string_view kTypeDecimalStr{"MYSQL_TYPE_DECIMAL"};
constexpr std::string_view kTypeNewDecimalStr{"MYSQL_TYPE_NEWDECIMAL"};
constexpr std::string_view kTypeTinyStr{"MYSQL_TYPE_TINY"};
constexpr std::string_view kTypeShortStr{"MYSQL_TYPE_SHORT"};
constexpr std::string_view kTypeLongStr{"MYSQL_TYPE_LONG"};
constexpr std::string_view kTypeFloatStr{"MYSQL_TYPE_FLOAT"};
constexpr std::string_view kTypeDoubleStr{"MYSQL_TYPE_DOUBLE"};
constexpr std::string_view kTypeTimestampStr{"MYSQL_TYPE_TIMESTAMP"};
constexpr std::string_view kTypeLongLongStr{"MYSQL_TYPE_LONGLONG"};
constexpr std::string_view kTypeInt24Str{"MYSQL_TYPE_INT24"};
constexpr std::string_view kTypeDateStr{"MYSQL_TYPE_DATE"};
constexpr std::string_view kTypeTimeStr{"MYSQL_TYPE_TIME"};
constexpr std::string_view kTypeDatetimeStr{"MYSQL_TYPE_DATETIME"};
constexpr std::string_view kTypeTinyBlobStr{"MYSQL_TYPE_TINY_BLOB"};
constexpr std::string_view kTypeMediumBlobStr{"MYSQL_TYPE_MEDIUM_BLOB"};
constexpr std::string_view kTypeLongBlobStr{"MYSQL_TYPE_LONG_BLOB"};
constexpr std::string_view kTypeBlobStr{"MYSQL_TYPE_BLOB"};
constexpr std::string_view kTypeVarStringStr{"MYSQL_TYPE_VAR_STRING"};
constexpr std::string_view kTypeStringStr{"MYSQL_TYPE_STRING"};
constexpr std::string_view kTypeUnknown{"UNKNOWN"};

// clang-format off
std::string_view FieldTypeToString(enum_field_types type) {
  using Type = enum_field_types;
  switch (type) {
    case Type::MYSQL_TYPE_DECIMAL: return kTypeDecimalStr;
    case Type::MYSQL_TYPE_NEWDECIMAL: return kTypeNewDecimalStr;
    case Type::MYSQL_TYPE_TINY: return kTypeTinyStr;
    case Type::MYSQL_TYPE_SHORT: return kTypeShortStr;
    case Type::MYSQL_TYPE_LONG: return kTypeLongStr;
    case Type::MYSQL_TYPE_FLOAT: return kTypeFloatStr;
    case Type::MYSQL_TYPE_DOUBLE: return kTypeDoubleStr;
    case Type::MYSQL_TYPE_TIMESTAMP: return kTypeTimestampStr;
    case Type::MYSQL_TYPE_LONGLONG: return kTypeLongLongStr;
    case Type::MYSQL_TYPE_INT24: return kTypeInt24Str;
    case Type::MYSQL_TYPE_DATE: return kTypeDateStr;
    case Type::MYSQL_TYPE_TIME: return kTypeTimeStr;
    case Type::MYSQL_TYPE_DATETIME: return kTypeDatetimeStr;
    case Type::MYSQL_TYPE_TINY_BLOB: return kTypeTinyBlobStr;
    case Type::MYSQL_TYPE_MEDIUM_BLOB: return kTypeMediumBlobStr;
    case Type::MYSQL_TYPE_LONG_BLOB: return kTypeLongBlobStr;
    case Type::MYSQL_TYPE_BLOB: return kTypeBlobStr;
    case Type::MYSQL_TYPE_VAR_STRING: return kTypeVarStringStr;
    case Type::MYSQL_TYPE_STRING: return kTypeStringStr;

    default: return kTypeUnknown;
  }
}
// clang-format on

}  // namespace

bool NativeBindsHelper::IsFieldNumeric(enum_field_types type) {
  return type == MYSQL_TYPE_TINY || type == MYSQL_TYPE_SHORT ||
         type == MYSQL_TYPE_INT24 || type == MYSQL_TYPE_LONG ||
         type == MYSQL_TYPE_LONG_BLOB || type == MYSQL_TYPE_FLOAT ||
         type == MYSQL_TYPE_DOUBLE || type == MYSQL_TYPE_DECIMAL;
}

std::string_view NativeBindsHelper::NativeTypeToString(enum_field_types type) {
  return FieldTypeToString(type);
}

MYSQL_TIME NativeBindsHelper::ToNativeTime(
    std::chrono::system_clock::time_point tp) {
  // https://stackoverflow.com/a/15958113/10508079
  auto dp = date::floor<date::days>(tp);
  auto ymd = date::year_month_day{date::floor<date::days>(tp)};
  auto time = date::make_time(
      std::chrono::duration_cast<MariaDBTimepointResolution>(tp - dp));

  MYSQL_TIME native_time{};
  native_time.year = ymd.year().operator int();
  native_time.month = ymd.month().operator unsigned int();
  native_time.day = ymd.day().operator unsigned int();
  native_time.hour = time.hours().count();
  native_time.minute = time.minutes().count();
  native_time.second = time.seconds().count();
  native_time.second_part = time.subseconds().count();

  native_time.time_type = MYSQL_TIMESTAMP_DATETIME;

  return native_time;
}

std::chrono::system_clock::time_point NativeBindsHelper::FromNativeTime(
    const MYSQL_TIME& native_time) {
  // https://stackoverflow.com/a/15958113/10508079
  const date::year year(native_time.year);
  const date::month month{native_time.month};
  const date::day day{native_time.day};
  const std::chrono::hours hour{native_time.hour};
  const std::chrono::minutes minute{native_time.minute};
  const std::chrono::seconds second{native_time.second};
  const MariaDBTimepointResolution microsecond{native_time.second_part};

  return date::sys_days{year / month / day} + hour + minute + second +
         microsecond;
}

}  // namespace storages::mysql::impl::bindings

USERVER_NAMESPACE_END
