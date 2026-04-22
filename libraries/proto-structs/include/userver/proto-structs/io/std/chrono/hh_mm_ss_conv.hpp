#pragma once

/// @file userver/proto-structs/io/std/chrono/hh_mm_ss_conv.hpp
/// @brief Provides read/write context class with the ability to handle `std::chrono::hh_mm_ss` conversion.

#include <userver/proto-structs/io/std/chrono/hh_mm_ss.hpp>

#include <google/type/timeofday.pb.h>

#include <userver/proto-structs/exceptions.hpp>
#include <userver/proto-structs/io/context.hpp>
#include <userver/proto-structs/time_of_day.hpp>
#include <userver/utils/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

template <typename TDuration>
std::chrono::hh_mm_ss<TDuration> ReadProtoStruct(
    ReadContext& ctx,
    To<std::chrono::hh_mm_ss<TDuration>>,
    const ::google::type::TimeOfDay& msg
) {
    // note that `google.type.TimeOfDay` allows 60 for seconds (for leap-seconds) in most general case, however this
    // will not be expected by most users (which will expect hms <= 24h), so we prefer to fail on such values
    if (msg.seconds() == 60) {
        ctx.AddError("leap seconds are not supported when converting 'google.type.TimeOfDay' to 'std::chrono::hh_mm_ss'"
        );
        return std::chrono::hh_mm_ss<TDuration>{};
    }

    try {
        return TimeOfDay(::utils::impl::InternalTag{}, msg.hours(), msg.minutes(), msg.seconds(), msg.nanos())
            .ToChronoTimeOfDay<TDuration>();
    } catch (const ValueError& e) {
        ctx.AddError(e.what());
        return std::chrono::hh_mm_ss<TDuration>{};
    }
}

template <typename TDuration>
void WriteProtoStruct(WriteContext& ctx, const std::chrono::hh_mm_ss<TDuration>& obj, ::google::type::TimeOfDay& msg) {
    try {
        TimeOfDay time_of_day(obj);
        msg.set_hours(static_cast<int32_t>(time_of_day.Hours().count()));
        msg.set_minutes(static_cast<int32_t>(time_of_day.Minutes().count()));
        msg.set_seconds(static_cast<int32_t>(time_of_day.Seconds().count()));
        msg.set_nanos(static_cast<int32_t>(time_of_day.Nanos().count()));
    } catch (const ValueError& e) {
        ctx.AddError(e.what());
    }
}

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
