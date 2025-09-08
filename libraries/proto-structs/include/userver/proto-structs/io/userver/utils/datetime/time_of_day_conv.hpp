#pragma once

/// @file userver/proto-structs/io/userver/utils/datetime/time_of_day_conv.hpp
/// @brief Provides read/write context class with the ability to handle `userver::utils::datetime::TimeOfDay` conversion

#include <userver/proto-structs/io/userver/utils/datetime/time_of_day.hpp>

#include <userver/proto-structs/io/std/chrono/hh_mm_ss_conv.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

template <typename TDuration>
utils::datetime::TimeOfDay<TDuration>
ReadProtoStruct(ReadContext& ctx, To<utils::datetime::TimeOfDay<TDuration>>, const ::google::type::TimeOfDay& msg) {
    auto hms = ReadProtoStruct(ctx, To<std::chrono::hh_mm_ss<TDuration>>{}, msg);
    return utils::datetime::TimeOfDay<TDuration>{hms.to_duration()};
}

template <typename TDuration>
void WriteProtoStruct(
    WriteContext& ctx,
    const utils::datetime::TimeOfDay<TDuration>& obj,
    ::google::type::TimeOfDay& msg
) {
    std::chrono::hh_mm_ss<TDuration> hms{obj.SinceMidnight()};
    WriteProtoStruct(ctx, hms, msg);
}

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
