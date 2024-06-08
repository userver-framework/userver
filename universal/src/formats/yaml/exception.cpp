#include <userver/formats/yaml/exception.hpp>
#include <userver/utils/algo.hpp>

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
std::string MsgForType(TType actual, TType expected) {
  UASSERT(actual != expected);
  return fmt::format(
      "Wrong type. Expected YAML "
      "type '{}', but found '{}'",
      NameForType(expected), NameForType(actual));
}

std::string MsgForType(std::string_view expected_type,
                       const YAML::Node& value) {
  return fmt::format(
      "Not representable as '{}'. Can "
      "not convert from value '{}'",
      expected_type, value.Scalar());
}

std::string MsgForIndex(size_t index, size_t size) {
  return fmt::format("Index {} of array of size {} is out of bounds", index,
                     size);
}

std::string MsgForPathPrefix(std::string_view old_path,
                             std::string_view prefix) {
  return fmt::format(
      "Attempting to append path prefix '{}' to a non-empty path '{}'", prefix,
      old_path);
}

constexpr std::string_view kErrorAtPath1 = "Error at path '";
constexpr std::string_view kErrorAtPath2 = "': ";

}  // namespace

ExceptionWithPath::ExceptionWithPath(std::string_view msg,
                                     std::string_view path)
    : Exception(utils::StrCat(kErrorAtPath1, path, kErrorAtPath2, msg)),
      path_size_(path.size()) {}

std::string_view ExceptionWithPath::GetPath() const noexcept {
  return GetMessage().substr(kErrorAtPath1.size(), path_size_);
}

std::string_view ExceptionWithPath::GetMessageWithoutPath() const noexcept {
  return GetMessage().substr(path_size_ + kErrorAtPath1.size() +
                             kErrorAtPath2.size());
}

BadStreamException::BadStreamException(const std::istream& is)
    : Exception(MsgForState(is.rdstate(), "input")) {}

BadStreamException::BadStreamException(const std::ostream& os)
    : Exception(MsgForState(os.rdstate(), "output")) {}

TypeMismatchException::TypeMismatchException(Type actual, Type expected,
                                             std::string_view path)
    : ExceptionWithPath(MsgForType(actual, expected), path) {}

TypeMismatchException::TypeMismatchException(int actual, int expected,
                                             std::string_view path)
    : ExceptionWithPath(MsgForType(static_cast<impl::Type>(actual),
                                   static_cast<impl::Type>(expected)),
                        path) {}

TypeMismatchException::TypeMismatchException(const YAML::Node& value,
                                             std::string_view expected_type,
                                             std::string_view path)
    : ExceptionWithPath(MsgForType(expected_type, value), path) {}

OutOfBoundsException::OutOfBoundsException(size_t index, size_t size,
                                           std::string_view path)
    : ExceptionWithPath(MsgForIndex(index, size), path) {}

MemberMissingException::MemberMissingException(std::string_view path)
    : ExceptionWithPath("Field is missing", path) {}

PathPrefixException::PathPrefixException(std::string_view old_path,
                                         std::string_view prefix)
    : Exception(MsgForPathPrefix(old_path, prefix)) {}

}  // namespace formats::yaml

USERVER_NAMESPACE_END
