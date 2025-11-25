#pragma once

/// @file userver/proto-structs/io/userver/proto_structs/timestamp_conv.hpp
/// @brief Provides read/write context class with the ability to handle `userver::proto_structs::Timestamp` conversion.

#include <userver/proto-structs/io/userver/proto_structs/timestamp.hpp>

#include <userver/proto-structs/io/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

Timestamp ReadProtoStruct(ReadContext&, To<Timestamp>, const ::google::protobuf::Timestamp&);

void WriteProtoStruct(WriteContext&, const Timestamp&, ::google::protobuf::Timestamp&);

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
