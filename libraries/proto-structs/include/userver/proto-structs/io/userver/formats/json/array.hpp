#pragma once

/// @file userver/proto-structs/io/userver/formats/json/array.hpp
/// @brief Provides @ref formats::json::Array proto struct field support.

#include <userver/formats/json/array.hpp>

#include <userver/proto-structs/type_mapping.hpp>

namespace google::protobuf {
class ListValue;
}  // namespace google::protobuf

USERVER_NAMESPACE_BEGIN

namespace proto_structs::traits {

template <>
struct CompatibleMessageTrait<formats::json::Array> {
    using Type = ::google::protobuf::ListValue;
};

}  // namespace proto_structs::traits

USERVER_NAMESPACE_END
