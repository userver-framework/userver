#pragma once

/// @file userver/proto-structs/io/userver/formats/json/value_conv.hpp
/// @brief Provides read/write context class with the ability to handle `userver::formats::json::Value` conversion.

#include <userver/proto-structs/io/userver/formats/json/value.hpp>

#include <userver/proto-structs/io/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

formats::json::Value ReadProtoStruct(ReadContext&, To<formats::json::Value>, const ::google::protobuf::Value&);

void WriteProtoStruct(WriteContext&, const formats::json::Value&, ::google::protobuf::Value&);

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
