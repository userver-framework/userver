#include <formats/yaml/exception.hpp>

#include <utils/assert.hpp>

#include <istream>
#include <ostream>

#include <yaml-cpp/yaml.h>

namespace formats {
namespace yaml {
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

const char* NameForType(Type expected) {
#define RET_NAME(type)          \
  if (Type::type == expected) { \
    return #type;               \
  }
  RET_NAME(kNull)
  RET_NAME(kArray)
  RET_NAME(kObject)
  RET_NAME(kMap)
  RET_NAME(kMissing)
  return "ERROR";
#undef RET_NAME
}

std::string MsgForType(Type actual, Type expected, const std::string& path) {
  std::string ret =
      std::string("Field '") + path +
      "' has wrong type. Expected YAML Value type: " + NameForType(expected) +
      ", actual: " + NameForType(actual);

  UASSERT(actual != expected);
  return ret;
}

std::string MsgForType(const std::string& expected_type,
                       const std::string& path, const YAML::Node& value) {
  std::string ret = std::string("Field '") + path +
                    "' is not representable as " + expected_type + ". ";
  ret += "Can not convert from value '" + value.Scalar() + "'";

  return ret;
}

std::string MsgForIndex(size_t index, size_t size, const std::string& path) {
  return std::string("Index ") + std::to_string(index) + " of array '" + path +
         "' of size " + std::to_string(size) + " is out of bounds";
}

std::string MsgForMissing(const std::string& path) {
  return std::string("Field '") + path + "' is missing";
}

template <typename T>
std::string MsgForOverflow(T min, T value, T max, const std::string& path) {
  return std::string("Value of '") + path + "' is out of bounds (" +
         std::to_string(min) + " <= " + std::to_string(value) +
         " <= " + std::to_string(max) + ")";
}

}  // namespace

BadStreamException::BadStreamException(const std::istream& is)
    : YamlException(MsgForState(is.rdstate(), "input")) {}

BadStreamException::BadStreamException(const std::ostream& os)
    : YamlException(MsgForState(os.rdstate(), "output")) {}

TypeMismatchException::TypeMismatchException(Type actual, Type expected,
                                             const std::string& path)
    : YamlException(MsgForType(actual, expected, path)) {}

TypeMismatchException::TypeMismatchException(const YAML::Node& value,
                                             const std::string& expected_type,
                                             const std::string& path)
    : YamlException(MsgForType(expected_type, path, value)) {}

OutOfBoundsException::OutOfBoundsException(size_t index, size_t size,
                                           const std::string& path)
    : YamlException(MsgForIndex(index, size, path)) {}

IntegralOverflowException::IntegralOverflowException(int64_t min, int64_t value,
                                                     int64_t max,
                                                     const std::string& path)
    : YamlException(MsgForOverflow(min, value, max, path)) {}

IntegralOverflowException::IntegralOverflowException(uint64_t min,
                                                     uint64_t value,
                                                     uint64_t max,
                                                     const std::string& path)
    : YamlException(MsgForOverflow(min, value, max, path)) {}

MemberMissingException::MemberMissingException(const std::string& path)
    : YamlException(MsgForMissing(path)) {}

}  // namespace yaml
}  // namespace formats
