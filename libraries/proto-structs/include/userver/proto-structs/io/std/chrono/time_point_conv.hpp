#pragma once

/// @file userver/proto-structs/io/std/chrono/time_point_conv.hpp
/// @brief Provides read/write context class with the ability to handle `std::chrono::time_point` conversion.

#include <userver/proto-structs/io/std/chrono/time_point.hpp>

#include <userver/proto-structs/io/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

std::chrono::
    time_point<std::chrono::system_clock>
    ReadProtoStruct(ReadContext&, To<std::chrono::time_point<std::chrono::system_clock>>, const ::google::protobuf::Timestamp&);

void WriteProtoStruct(WriteContext&, const std::chrono::time_point<std::chrono::system_clock>&, ::google::protobuf::Timestamp&);

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
