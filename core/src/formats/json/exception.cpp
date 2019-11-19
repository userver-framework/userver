#include <formats/json/exception.hpp>

#include <istream>
#include <ostream>

#include <json/value.h>
#include <rapidjson/document.h>

#include "exttypes.hpp"

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

  return std::string("The ") + stream + " stream is in state '" + str_state +
         '\'';
}

template <typename TType>
std::string MsgForType(TType actual, TType expected, const std::string& path) {
  return std::string("Field '") + path +
         "' is of a wrong type. Expected: " + NameForType(expected) +
         ", actual: " + NameForType(actual);
}

std::string MsgForIndex(size_t index, size_t size, const std::string& path) {
  return std::string("Index ") + std::to_string(index) + " of array '" + path +
         "' of size " + std::to_string(size) + " is out of bounds";
}

std::string MsgForMissing(const std::string& path) {
  return std::string("Field '") + path + "' is missing";
}

}  // namespace

namespace formats {
namespace json {

BadStreamException::BadStreamException(const std::istream& is)
    : Exception(MsgForState(is.rdstate(), "input")) {}

BadStreamException::BadStreamException(const std::ostream& os)
    : Exception(MsgForState(os.rdstate(), "output")) {}

TypeMismatchException::TypeMismatchException(int actual, int expected,
                                             const std::string& path)
    : Exception(MsgForType(static_cast<impl::Type>(actual),
                           static_cast<impl::Type>(expected), path)) {}

OutOfBoundsException::OutOfBoundsException(size_t index, size_t size,
                                           const std::string& path)
    : Exception(MsgForIndex(index, size, path)) {}

MemberMissingException::MemberMissingException(const std::string& path)
    : Exception(MsgForMissing(path)) {}

}  // namespace json
}  // namespace formats
