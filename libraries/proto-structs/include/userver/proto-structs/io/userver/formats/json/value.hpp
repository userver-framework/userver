#pragma once

/// @file userver/proto-structs/io/userver/formats/json/value.hpp
/// @brief Provides @ref formats::json::Value proto struct field support.

#include <userver/formats/json/value.hpp>

#include <userver/proto-structs/type_mapping.hpp>

namespace google::protobuf {
class Value;
}  // namespace google::protobuf

USERVER_NAMESPACE_BEGIN

namespace proto_structs::traits {

template <>
struct CompatibleMessageTrait<formats::json::Value> {
    using Type = ::google::protobuf::Value;
};

}  // namespace proto_structs::traits

USERVER_NAMESPACE_END
