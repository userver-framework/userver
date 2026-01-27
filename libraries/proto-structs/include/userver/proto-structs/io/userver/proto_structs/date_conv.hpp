#pragma once

/// @file userver/proto-structs/io/userver/proto_structs/date_conv.hpp
/// @brief Provides read/write context class with the ability to handle `userver::proto_structs::Date` conversion.

#include <userver/proto-structs/io/userver/proto_structs/date.hpp>

#include <userver/proto-structs/io/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

Date ReadProtoStruct(ReadContext&, To<Date>, const ::google::type::Date&);

void WriteProtoStruct(WriteContext&, const Date&, ::google::type::Date&);

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
