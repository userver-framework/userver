#include "exttypes.hpp"

USERVER_NAMESPACE_BEGIN

namespace formats::json::impl {
Type GetExtendedType(const Value& val) {
    switch (val.GetType()) {
        case rapidjson::kNullType:
            return kNullValue;
        case rapidjson::kObjectType:
            return kObjectValue;
        case rapidjson::kArrayType:
            return kArrayValue;
        case rapidjson::kStringType:
            return kStringValue;
        case rapidjson::kTrueType:
        case rapidjson::kFalseType:
            return kBooleanValue;
        case rapidjson::kNumberType:
            if (val.IsInt64()) {
                return kIntValue;
            }
            if (val.IsUint64()) {
                return kUintValue;
            }
            return kRealValue;
    }
    return kErrorValue;
}

const char* NameForType(Type expected) {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage): XX-style macro
#define RET_NAME(type) \
    case Type::type:   \
        return #type;
    switch (expected) {
        RET_NAME(kNullValue);
        RET_NAME(kIntValue);
        RET_NAME(kUintValue);
        RET_NAME(kRealValue);
        RET_NAME(kStringValue);
        RET_NAME(kBooleanValue);
        RET_NAME(kArrayValue);
        RET_NAME(kObjectValue);
        RET_NAME(kErrorValue);
    }
    return "ERROR";
#undef RET_NAME
}
}  // namespace formats::json::impl

USERVER_NAMESPACE_END
