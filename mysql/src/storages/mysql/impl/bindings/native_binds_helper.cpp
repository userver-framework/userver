#include <storages/mysql/impl/bindings/native_binds_helper.hpp>

#include <storages/mysql/impl/time_utils.hpp>

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
  return type == MYSQL_TYPE_DECIMAL || type == MYSQL_TYPE_NEWDECIMAL ||
         type == MYSQL_TYPE_TINY || type == MYSQL_TYPE_SHORT ||
         type == MYSQL_TYPE_INT24 || type == MYSQL_TYPE_LONG ||
         type == MYSQL_TYPE_LONGLONG || type == MYSQL_TYPE_FLOAT ||
         type == MYSQL_TYPE_DOUBLE;
  // TODO : MYSQL_TYPE_BIT?
}

std::size_t NativeBindsHelper::NumericFieldWidth(enum_field_types type) {
  UASSERT(IsFieldNumeric(type));

  switch (type) {
    case MYSQL_TYPE_DECIMAL:
    case MYSQL_TYPE_NEWDECIMAL:
      // it's 8 bytes in userver for all we care,
      // this branch should be unreachable anyway
      return 8;
    case MYSQL_TYPE_TINY:
      return 1;
    case MYSQL_TYPE_SHORT:
      return 2;
    case MYSQL_TYPE_INT24:
      return 3;
    case MYSQL_TYPE_LONG:
      return 4;
    case MYSQL_TYPE_LONGLONG:
      return 8;
    case MYSQL_TYPE_FLOAT:
      return 4;
    case MYSQL_TYPE_DOUBLE:
      return 8;
    default:
      UINVARIANT(false, "should be unreachable");
  }
}

std::string_view NativeBindsHelper::NativeTypeToString(enum_field_types type) {
  return FieldTypeToString(type);
}

MYSQL_TIME NativeBindsHelper::ToNativeTime(
    std::chrono::system_clock::time_point tp) {
  return TimeUtils::ToNativeTime(tp);
}

std::chrono::system_clock::time_point NativeBindsHelper::FromNativeTime(
    const MYSQL_TIME& native_time) {
  return TimeUtils::FromNativeTime(native_time);
}

}  // namespace storages::mysql::impl::bindings

USERVER_NAMESPACE_END
