#pragma once

/// @file userver/proto-structs/io/userver/formats/json/array_conv.hpp
/// @brief Provides read/write context class with the ability to handle `userver::formats::json::Array` conversion.

#include <userver/proto-structs/io/userver/formats/json/array.hpp>

#include <userver/proto-structs/io/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

formats::json::Array ReadProtoStruct(ReadContext&, To<formats::json::Array>, const ::google::protobuf::ListValue&);

void WriteProtoStruct(WriteContext&, const formats::json::Array&, ::google::protobuf::ListValue&);

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
