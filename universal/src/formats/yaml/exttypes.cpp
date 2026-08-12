#include <userver/formats/yaml/value.hpp>

#include "exttypes.hpp"

USERVER_NAMESPACE_BEGIN

namespace formats::yaml::impl {
Type GetExtendedType(const YAML::Node& val) {
    switch (val.Type()) {
        case YAML::NodeType::Sequence:
            return kArrayValue;
        case YAML::NodeType::Map:
            return kObjectValue;
        case YAML::NodeType::Null:
            return kNullValue;
        case YAML::NodeType::Scalar:
            return kScalarValue;
        case YAML::NodeType::Undefined:
            throw std::logic_error("undefined node type should not be used");
    }
    return kErrorValue;
}

const char* NameForType(Type expected) {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage): XX-style macro
#define RET_NAME(type) \
    case Type::type:   \
        return #type;
    switch (expected) {
        RET_NAME(kNullValue)
        RET_NAME(kScalarValue)
        RET_NAME(kArrayValue)
        RET_NAME(kObjectValue)
        RET_NAME(kErrorValue)
    }
    return "ERROR";
#undef RET_NAME
}
}  // namespace formats::yaml::impl

USERVER_NAMESPACE_END
