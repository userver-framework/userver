#include <userver/formats/yaml/value.hpp>

#include "exttypes.hpp"

USERVER_NAMESPACE_BEGIN

namespace formats::yaml::impl {
Type GetExtendedType(const YAML::Node& val) {
  switch (val.Type()) {
    case YAML::NodeType::Sequence:
      return arrayValue;
    case YAML::NodeType::Map:
      return objectValue;
    case YAML::NodeType::Null:
      return nullValue;
    case YAML::NodeType::Scalar:
      return scalarValue;
    case YAML::NodeType::Undefined:
      throw std::logic_error("undefined node type should not be used");
  }
  return errorValue;
}

const char* NameForType(Type expected) {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage): XX-style macro
#define RET_NAME(type) \
  case Type::type:     \
    return #type;
  switch (expected) {
    RET_NAME(nullValue)
    RET_NAME(scalarValue)
    RET_NAME(arrayValue)
    RET_NAME(objectValue)
    RET_NAME(errorValue)
  }
  return "ERROR";
#undef RET_NAME
}
}  // namespace formats::yaml::impl

USERVER_NAMESPACE_END
