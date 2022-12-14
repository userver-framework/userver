#include <userver/formats/bson/exception.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace formats::bson {
namespace {

const char* NameForType(bson_type_t type) {
  switch (type) {
    case BSON_TYPE_EOD:
      return "EOD";
    case BSON_TYPE_DOUBLE:
      return "DOUBLE";
    case BSON_TYPE_UTF8:
      return "UTF8";
    case BSON_TYPE_DOCUMENT:
      return "DOCUMENT";
    case BSON_TYPE_ARRAY:
      return "ARRAY";
    case BSON_TYPE_BINARY:
      return "BINARY";
    case BSON_TYPE_UNDEFINED:
      return "UNDEFINED";
    case BSON_TYPE_OID:
      return "OID";
    case BSON_TYPE_BOOL:
      return "BOOL";
    case BSON_TYPE_DATE_TIME:
      return "DATE_TIME";
    case BSON_TYPE_NULL:
      return "NULL";
    case BSON_TYPE_REGEX:
      return "REGEX";
    case BSON_TYPE_DBPOINTER:
      return "DBPOINTER";
    case BSON_TYPE_CODE:
      return "CODE";
    case BSON_TYPE_SYMBOL:
      return "SYMBOL";
    case BSON_TYPE_CODEWSCOPE:
      return "CODEWSCOPE";
    case BSON_TYPE_INT32:
      return "INT32";
    case BSON_TYPE_TIMESTAMP:
      return "TIMESTAMP";
    case BSON_TYPE_INT64:
      return "INT64";
    case BSON_TYPE_MAXKEY:
      return "MAXKEY";
    case BSON_TYPE_MINKEY:
      return "MINKEY";
    case BSON_TYPE_DECIMAL128:
      return "DECIMAL128";
  }
}

std::string MsgForType(bson_type_t actual, bson_type_t expected,
                       std::string_view path) {
  return fmt::format("Field '{}' is of a wrong type. Expected: {}, actual: {}",
                     path, NameForType(expected), NameForType(actual));
}

std::string MsgForIndex(size_t index, size_t size, std::string_view path) {
  return fmt::format("Index {} of array '{}' of size {} is out of bounds",
                     index, path, size);
}

std::string MsgForMissing(std::string_view path) {
  return fmt::format("Field '{}' is missing", path);
}

}  // namespace

TypeMismatchException::TypeMismatchException(bson_type_t actual,
                                             bson_type_t expected,
                                             std::string_view path)
    : BsonException(MsgForType(actual, expected, path)) {}

OutOfBoundsException::OutOfBoundsException(size_t index, size_t size,
                                           std::string_view path)
    : BsonException(MsgForIndex(index, size, path)) {}

MemberMissingException::MemberMissingException(std::string_view path)
    : BsonException(MsgForMissing(path)) {}

}  // namespace formats::bson

USERVER_NAMESPACE_END
