#include <userver/formats/yaml/exception.hpp>

#include <fmt/format.h>

#include <userver/utils/assert.hpp>

#include <istream>
#include <ostream>

#include <yaml-cpp/yaml.h>

#include "exttypes.hpp"

USERVER_NAMESPACE_BEGIN

namespace formats::yaml {
namespace {

std::string MsgForState(std::ios::iostate state, std::string_view stream_kind) {
  std::string_view str_state;
  if (state & std::ios::badbit) {
    str_state = "BAD";
  } else if (state & std::ios::failbit) {
    str_state = "FAIL";
  } else if (state & std::ios::eofbit) {
    str_state = "EOF";
  } else {
    str_state = "GOOD?";
  }

  return fmt::format("The {} stream is in state '{}'", stream_kind, str_state);
}

const char* NameForType(Type expected) {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage): XX-style macro
#define RET_NAME(type)          \
  if (Type::type == expected) { \
    return #type;               \
  }
  RET_NAME(kNull)
  RET_NAME(kArray)
  RET_NAME(kObject)
  return "ERROR";
#undef RET_NAME
}

template <typename TType>
std::string MsgForType(TType actual, TType expected, std::string_view path) {
  UASSERT(actual != expected);
  return fmt::format(
      "Field '{}' is of wrong type. Expected YAML "
      "type '{}', but found '{}'",
      path, NameForType(expected), NameForType(actual));
}

std::string MsgForType(std::string_view expected_type, std::string_view path,
                       const YAML::Node& value) {
  return fmt::format(
      "Field '{}' is not representable as '{}'. Can "
      "not convert from value '{}'",
      path, expected_type, value.Scalar());
}

std::string MsgForIndex(size_t index, size_t size, std::string_view path) {
  return fmt::format("Index {} of array '{}' of size {} is out of bounds",
                     index, path, size);
}

std::string MsgForMissing(std::string_view path) {
  return fmt::format("Field '{}' is missing", path);
}

std::string MsgForPathPrefix(std::string_view old_path,
                             std::string_view prefix) {
  return fmt::format(
      "Attempting to append path prefix '{}' to a non-empty path '{}'", prefix,
      old_path);
}

}  // namespace

BadStreamException::BadStreamException(const std::istream& is)
    : Exception(MsgForState(is.rdstate(), "input")) {}

BadStreamException::BadStreamException(const std::ostream& os)
    : Exception(MsgForState(os.rdstate(), "output")) {}

TypeMismatchException::TypeMismatchException(Type actual, Type expected,
                                             std::string_view path)
    : Exception(MsgForType(actual, expected, path)) {}

TypeMismatchException::TypeMismatchException(int actual, int expected,
                                             std::string_view path)
    : Exception(MsgForType(static_cast<impl::Type>(actual),
                           static_cast<impl::Type>(expected), path)) {}

TypeMismatchException::TypeMismatchException(const YAML::Node& value,
                                             std::string_view expected_type,
                                             std::string_view path)
    : Exception(MsgForType(expected_type, path, value)) {}

OutOfBoundsException::OutOfBoundsException(size_t index, size_t size,
                                           std::string_view path)
    : Exception(MsgForIndex(index, size, path)) {}

MemberMissingException::MemberMissingException(std::string_view path)
    : Exception(MsgForMissing(path)) {}

PathPrefixException::PathPrefixException(std::string_view old_path,
                                         std::string_view prefix)
    : Exception(MsgForPathPrefix(old_path, prefix)) {}

}  // namespace formats::yaml

USERVER_NAMESPACE_END
