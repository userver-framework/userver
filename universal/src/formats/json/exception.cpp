#include <userver/formats/json/exception.hpp>

#include <istream>
#include <ostream>
#include <string_view>

#include <fmt/format.h>
#include <rapidjson/document.h>

#include <formats/json/impl/exttypes.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

std::string MsgForState(std::ios::iostate state, const char* stream) {
  const char* str_state = "GOOD?";
  if (state & std::ios::badbit) {
    str_state = "BAD";
  } else if (state & std::ios::failbit) {
    str_state = "FAIL";
  } else if (state & std::ios::eofbit) {
    str_state = "EOF";
  }

  return fmt::format("The {} stream is in state '{}'", stream, str_state);
}

template <typename TType>
std::string MsgForType(TType actual, TType expected, const std::string& path) {
  return fmt::format("Field '{}' is of a wrong type. Expected: {}, actual: {}",
                     path, NameForType(expected), NameForType(actual));
}

std::string MsgForIndex(size_t index, size_t size, const std::string& path) {
  return fmt::format("Index {} of array '{}' of size {} is out of bounds",
                     index, path, size);
}

std::string MsgForMissing(const std::string& path) {
  return fmt::format("Field '{}' is missing", path);
}

}  // namespace

namespace formats::json {

BadStreamException::BadStreamException(const std::istream& is)
    : Exception(MsgForState(is.rdstate(), "input")) {}

BadStreamException::BadStreamException(const std::ostream& os)
    : Exception(MsgForState(os.rdstate(), "output")) {}

TypeMismatchException::TypeMismatchException(int actual, int expected,
                                             const std::string& path)
    : Exception(MsgForType(static_cast<impl::Type>(actual),
                           static_cast<impl::Type>(expected), path)),
      actual_(actual),
      expected_(expected),
      path_(path) {}

std::string_view TypeMismatchException::GetActual() const {
  return impl::NameForType(static_cast<impl::Type>(actual_));
}
std::string_view TypeMismatchException::GetExpected() const {
  return impl::NameForType(static_cast<impl::Type>(expected_));
}
const std::string& TypeMismatchException::GetPath() const noexcept {
  return path_;
}

OutOfBoundsException::OutOfBoundsException(size_t index, size_t size,
                                           const std::string& path)
    : Exception(MsgForIndex(index, size, path)), path_(path) {}

const std::string& OutOfBoundsException::GetPath() const noexcept {
  return path_;
}

MemberMissingException::MemberMissingException(const std::string& path)
    : Exception(MsgForMissing(path)) {}

}  // namespace formats::json

USERVER_NAMESPACE_END
