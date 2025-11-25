#pragma once

/// @file userver/proto-structs/io/userver/proto_structs/any_conv.hpp
/// @brief Provides read/write context class with the ability to handle `userver::proto_structs::Any` conversion.

#include <userver/proto-structs/io/userver/proto_structs/any.hpp>

#include <userver/proto-structs/io/fwd.hpp>

namespace google::protobuf {
class Any;
}  // namespace google::protobuf

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

proto_structs::Any ReadProtoStruct(ReadContext&, To<proto_structs::Any>, const ::google::protobuf::Any&);

void WriteProtoStruct(WriteContext&, const proto_structs::Any&, ::google::protobuf::Any&);

void WriteProtoStruct(WriteContext&, proto_structs::Any&&, ::google::protobuf::Any&);

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
