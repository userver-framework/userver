#pragma once

/// @file userver/proto-structs/io/std/chrono/year_month_day_conv.hpp
/// @brief Provides read/write context class with the ability to handle `std::chrono::year_month_day` conversion.

#include <userver/proto-structs/io/std/chrono/year_month_day.hpp>

#include <userver/proto-structs/io/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

std::chrono::year_month_day ReadProtoStruct(ReadContext&, To<std::chrono::year_month_day>, const ::google::type::Date&);

void WriteProtoStruct(WriteContext&, const std::chrono::year_month_day&, ::google::type::Date&);

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
