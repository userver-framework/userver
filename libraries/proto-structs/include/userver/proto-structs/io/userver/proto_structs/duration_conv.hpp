#pragma once

/// @file userver/proto-structs/io/userver/proto_structs/duration_conv.hpp
/// @brief Provides read/write context class with the ability to handle `userver::proto_structs::Duration` conversion.

#include <userver/proto-structs/io/userver/proto_structs/duration.hpp>

#include <userver/proto-structs/io/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

Duration ReadProtoStruct(ReadContext&, To<Duration>, const ::google::protobuf::Duration&);

void WriteProtoStruct(WriteContext&, const Duration&, ::google::protobuf::Duration&);

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
