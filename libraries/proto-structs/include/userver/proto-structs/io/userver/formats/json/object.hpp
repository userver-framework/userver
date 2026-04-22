#pragma once

/// @file userver/proto-structs/io/userver/formats/json/object.hpp
/// @brief Provides @ref formats::json::Object proto struct field support.

#include <userver/formats/json/object.hpp>

#include <userver/proto-structs/type_mapping.hpp>

namespace google::protobuf {
class Struct;
}  // namespace google::protobuf

USERVER_NAMESPACE_BEGIN

namespace proto_structs::traits {

template <>
struct CompatibleMessageTrait<formats::json::Object> {
    using Type = ::google::protobuf::Struct;
};

}  // namespace proto_structs::traits

USERVER_NAMESPACE_END
