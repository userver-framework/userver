#include <userver/formats/bson/exception.hpp>
#include <userver/utils/algo.hpp>

#include <fmt/format.h>

#include <userver/utils/assert.hpp>
#include <userver/utils/underlying_value.hpp>

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
  UINVARIANT(false, fmt::format("Invalid bson_type_t: {}",
                                utils::UnderlyingValue(type)));
}

std::string MsgForType(bson_type_t actual, bson_type_t expected) {
  return fmt::format("Wrong type. Expected: {}, actual: {}",
                     NameForType(expected), NameForType(actual));
}

std::string MsgForIndex(size_t index, size_t size) {
  return fmt::format("Index {} of array of size {} is out of bounds", index,
                     size);
}

constexpr std::string_view kErrorAtPath1 = "Error at path '";
constexpr std::string_view kErrorAtPath2 = "': ";

}  // namespace

BsonException::BsonException(std::string msg)
    : utils::TracefulException(msg), msg_(std::move(msg)) {}

ExceptionWithPath::ExceptionWithPath(std::string_view msg,
                                     std::string_view path)
    : BsonException(utils::StrCat(kErrorAtPath1, path, kErrorAtPath2, msg)),
      path_size_(path.size()) {}

std::string_view ExceptionWithPath::GetPath() const noexcept {
  return GetMessage().substr(kErrorAtPath1.size(), path_size_);
}

std::string_view ExceptionWithPath::GetMessageWithoutPath() const noexcept {
  return GetMessage().substr(path_size_ + kErrorAtPath1.size() +
                             kErrorAtPath2.size());
}

TypeMismatchException::TypeMismatchException(bson_type_t actual,
                                             bson_type_t expected,
                                             std::string_view path)
    : ExceptionWithPath(MsgForType(actual, expected), path) {}

OutOfBoundsException::OutOfBoundsException(size_t index, size_t size,
                                           std::string_view path)
    : ExceptionWithPath(MsgForIndex(index, size), path) {}

MemberMissingException::MemberMissingException(std::string_view path)
    : ExceptionWithPath("Field is missing", path) {}

ConversionException::ConversionException(std::string_view msg,
                                         std::string_view path)
    : ExceptionWithPath(msg, path) {}

}  // namespace formats::bson

USERVER_NAMESPACE_END
