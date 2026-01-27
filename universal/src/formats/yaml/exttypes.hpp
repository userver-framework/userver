#pragma once
// extended types to allow extract more precie type information from rapidyaml's
// Value

#include <yaml-cpp/yaml.h>
#include <userver/formats/yaml/types.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::yaml::impl {
enum Type {
    // order is important, it matches order in yaml-cpp/node/type.h
    kNullValue = YAML::NodeType::Null,
    kScalarValue = YAML::NodeType::Scalar,
    kArrayValue = YAML::NodeType::Sequence,
    kObjectValue = YAML::NodeType::Map,
    kErrorValue
};

Type GetExtendedType(const YAML::Node& val);
const char* NameForType(Type expected);
}  // namespace formats::yaml::impl

USERVER_NAMESPACE_END
