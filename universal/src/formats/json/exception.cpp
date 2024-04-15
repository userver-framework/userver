#include <userver/formats/json/exception.hpp>
#include <userver/utils/algo.hpp>

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
std::string MsgForType(TType actual, TType expected) {
  return fmt::format("Wrong type. Expected: {}, actual: {}",
                     NameForType(expected), NameForType(actual));
}

std::string MsgForIndex(size_t index, size_t size) {
  return fmt::format("Index {} of array of size {} is out of bounds", index,
                     size);
}

std::string MsgForUnknownDiscriminator(std::string_view discriminator_value) {
  return fmt::format("Unknown discriminator field value '{}'",
                     discriminator_value);
}

constexpr std::string_view kErrorAtPath1 = "Error at path '";
constexpr std::string_view kErrorAtPath2 = "': ";

}  // namespace

namespace formats::json {

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

TypeMismatchException::TypeMismatchException(int actual, int expected,
                                             std::string_view path)
    : ExceptionWithPath(MsgForType(static_cast<impl::Type>(actual),
                                   static_cast<impl::Type>(expected)),
                        path),
      actual_(actual),
      expected_(expected) {}

std::string_view TypeMismatchException::GetActual() const {
  return impl::NameForType(static_cast<impl::Type>(actual_));
}
std::string_view TypeMismatchException::GetExpected() const {
  return impl::NameForType(static_cast<impl::Type>(expected_));
}

OutOfBoundsException::OutOfBoundsException(size_t index, size_t size,
                                           std::string_view path)
    : ExceptionWithPath(MsgForIndex(index, size), path) {}

MemberMissingException::MemberMissingException(std::string_view path)
    : ExceptionWithPath("Field is missing", path) {}

ConversionException::ConversionException(std::string_view msg,
                                         std::string_view path)
    : ExceptionWithPath(msg, path) {}

UnknownDiscriminatorException::UnknownDiscriminatorException(
    std::string_view path, std::string_view discriminator_field)
    : ExceptionWithPath(MsgForUnknownDiscriminator(discriminator_field), path) {
}

}  // namespace formats::json

USERVER_NAMESPACE_END
