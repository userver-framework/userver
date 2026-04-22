#pragma once

/// @file userver/proto-structs/io/userver/formats/json/object_conv.hpp
/// @brief Provides read/write context class with the ability to handle `userver::formats::json::Object` conversion.

#include <userver/proto-structs/io/userver/formats/json/object.hpp>

#include <userver/proto-structs/io/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

formats::json::Object ReadProtoStruct(ReadContext&, To<formats::json::Object>, const ::google::protobuf::Struct&);

void WriteProtoStruct(WriteContext&, const formats::json::Object&, ::google::protobuf::Struct&);

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
