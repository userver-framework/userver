#pragma once

/// @file userver/proto-structs/io/userver/proto_structs/time_of_day_conv.hpp
/// @brief Provides read/write context class with the ability to handle `userver::proto_structs::TimeOfDay` conversion.

#include <userver/proto-structs/io/userver/proto_structs/time_of_day.hpp>

#include <userver/proto-structs/io/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

TimeOfDay ReadProtoStruct(ReadContext&, To<TimeOfDay>, const ::google::type::TimeOfDay&);

void WriteProtoStruct(WriteContext&, const TimeOfDay&, ::google::type::TimeOfDay&);

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
