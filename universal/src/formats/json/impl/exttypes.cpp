#include "exttypes.hpp"

USERVER_NAMESPACE_BEGIN

namespace formats::json::impl {
Type GetExtendedType(const Value& val) {
  switch (val.GetType()) {
    case rapidjson::kNullType:
      return nullValue;
    case rapidjson::kObjectType:
      return objectValue;
    case rapidjson::kArrayType:
      return arrayValue;
    case rapidjson::kStringType:
      return stringValue;
    case rapidjson::kTrueType:
    case rapidjson::kFalseType:
      return booleanValue;
    case rapidjson::kNumberType:
      if (val.IsInt64()) return intValue;
      if (val.IsUint64()) return uintValue;
      return realValue;
  }
  return errorValue;
}

const char* NameForType(Type expected) {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage): XX-style macro
#define RET_NAME(type) \
  case Type::type:     \
    return #type;
  switch (expected) {
    RET_NAME(nullValue);
    RET_NAME(intValue);
    RET_NAME(uintValue);
    RET_NAME(realValue);
    RET_NAME(stringValue);
    RET_NAME(booleanValue);
    RET_NAME(arrayValue);
    RET_NAME(objectValue);
    RET_NAME(errorValue);
  }
  return "ERROR";
#undef RET_NAME
}
}  // namespace formats::json::impl

USERVER_NAMESPACE_END
