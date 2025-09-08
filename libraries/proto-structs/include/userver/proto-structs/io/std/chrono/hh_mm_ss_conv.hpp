#pragma once

/// @file userver/proto-structs/io/std/chrono/hh_mm_ss_conv.hpp
/// @brief Provides read/write context class with the ability to handle `std::chrono::hh_mm_ss` conversion

#include <userver/proto-structs/io/std/chrono/hh_mm_ss.hpp>

#include <cstdint>

#include <fmt/format.h>
#include <google/type/timeofday.pb.h>

#include <userver/proto-structs/io/context.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

template <typename TDuration>
std::chrono::hh_mm_ss<TDuration>
ReadProtoStruct(ReadContext& ctx, To<std::chrono::hh_mm_ss<TDuration>>, const ::google::type::TimeOfDay& msg) {
    using Duration = TDuration;
    std::chrono::hh_mm_ss<Duration> result;

    // note that `google.type.TimeOfDay` allows 24 for hours (for business closing time) and 60 for seconds (for
    // leap-seconds) in most general case, however this will not be expected by most users, so we prefer to fail on
    // such values
    if ((msg.hours() < 0 || msg.hours() > 23) || (msg.minutes() < 0 || msg.minutes() > 59) ||
        (msg.seconds() < 0 || msg.seconds() > 59) || (msg.nanos() < 0 || msg.nanos() > 999'999'999)) {
        ctx.AddError(fmt::format(
            "{}:{}:{}.{} does not represent a valid time of a day",
            msg.hours(),
            msg.minutes(),
            msg.seconds(),
            msg.nanos()
        ));
        return result;
    }

    Duration duration = std::chrono::duration_cast<Duration>(
        std::chrono::hours(msg.hours()) + std::chrono::minutes(msg.minutes()) + std::chrono::seconds(msg.seconds()) +
        std::chrono::nanoseconds(msg.nanos())
    );
    result = std::chrono::hh_mm_ss<Duration>{duration};
    return result;
}

template <typename TDuration>
void WriteProtoStruct(WriteContext& ctx, const std::chrono::hh_mm_ss<TDuration>& obj, ::google::type::TimeOfDay& msg) {
    if (obj.is_negative() || obj.hours().count() > 23) {
        ctx.AddError(fmt::format(
            "{}:{}:{}.{} does not represent a valid time of a day",
            obj.hours().count(),
            obj.minutes().count(),
            obj.seconds().count(),
            obj.subseconds().count()
        ));
    }

    msg.set_hours(static_cast<int32_t>(obj.hours().count()));
    msg.set_minutes(static_cast<int32_t>(obj.minutes().count()));
    msg.set_seconds(static_cast<int32_t>(obj.seconds().count()));
    msg.set_nanos(static_cast<int32_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(obj.subseconds()).count()));
}

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
