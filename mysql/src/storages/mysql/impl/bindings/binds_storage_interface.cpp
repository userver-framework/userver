#include <storages/mysql/impl/bindings/binds_storage_interface.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::bindings {

namespace {

constexpr std::string_view kTypeDecimalStr{"MYSQL_TYPE_DECIMAL"};
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
         type == MYSQL_TYPE_DOUBLE;
}

std::string_view NativeBindsHelper::NativeTypeToString(enum_field_types type) {
  return FieldTypeToString(type);
}

}  // namespace storages::mysql::impl::bindings

USERVER_NAMESPACE_END
