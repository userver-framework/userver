#include <formats/json/exception.hpp>

#include <istream>
#include <ostream>

namespace {

std::string MsgForState(std::ios::iostate state, const char* stream) {
  const char* str_state = "GOOD?";
  if (state | std::ios::badbit) {
    str_state = "BAD";
  } else if (state | std::ios::failbit) {
    str_state = "FAIL";
  } else if (state | std::ios::eofbit) {
    str_state = "EOF";
  }

  return std::string("The ") + stream + " stream is in state '" + str_state +
         '\'';
}

const char* NameForType(Json::ValueType expected) {
#define RET_NAME(type)          \
  if (Json::type == expected) { \
    return #type;               \
  }
  RET_NAME(nullValue);
  RET_NAME(intValue);
  RET_NAME(uintValue);
  RET_NAME(realValue);
  RET_NAME(stringValue);
  RET_NAME(booleanValue);
  RET_NAME(arrayValue);
  RET_NAME(objectValue);
  return "ERROR";
#undef RET_NAME
}

std::string MsgForType(const Json::Value& doc, Json::ValueType expected,
                       const std::string& path) {
  return std::string("Field '") + path +
         "' is of a wrong type. Expected: " + NameForType(expected) +
         ", actual: " + NameForType(doc.type());
}

std::string MsgForIndex(size_t index, size_t size, const std::string& path) {
  return std::string("Index ") + std::to_string(index) + " of array '" + path +
         "' of size " + std::to_string(size) + " is out of bounds";
}

std::string MsgForKey(const std::string& key, const std::string& path) {
  return std::string("Field '") + key + "' of object '" + path + "' is missing";
}

}  // namespace

namespace formats {
namespace json {

BadStreamException::BadStreamException(const std::istream& is)
    : JsonException(MsgForState(is.rdstate(), "input")) {}

BadStreamException::BadStreamException(const std::ostream& os)
    : JsonException(MsgForState(os.rdstate(), "output")) {}

TypeMismatchException::TypeMismatchException(const Json::Value& doc,
                                             Json::ValueType expected,
                                             const std::string& path)
    : JsonException(MsgForType(doc, expected, path)) {}

OutOfBoundsException::OutOfBoundsException(size_t index, size_t size,
                                           const std::string& path)
    : JsonException(MsgForIndex(index, size, path)) {}

MemberMissingException::MemberMissingException(const std::string& key,
                                               const std::string& path)
    : JsonException(MsgForKey(key, path)) {}

}  // namespace json
}  // namespace formats
