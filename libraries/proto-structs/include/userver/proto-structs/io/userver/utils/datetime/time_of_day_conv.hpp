#pragma once

/// @file userver/proto-structs/io/userver/utils/datetime/time_of_day_conv.hpp
/// @brief Provides read/write context class with the ability to handle `userver::utils::datetime::TimeOfDay`
/// conversion.

#include <userver/proto-structs/io/userver/utils/datetime/time_of_day.hpp>

#include <userver/proto-structs/exceptions.hpp>
#include <userver/proto-structs/io/context.hpp>
#include <userver/proto-structs/time_of_day.hpp>
#include <userver/utils/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

template <typename TDuration>
utils::datetime::TimeOfDay<TDuration> ReadProtoStruct(
    ReadContext& ctx,
    To<utils::datetime::TimeOfDay<TDuration>>,
    const ::google::type::TimeOfDay& msg
) {
    // note that `google.type.TimeOfDay` allows 60 for seconds (for leap-seconds) and 24h in most general case,
    // however this will not be expected by most users (which will expect userver::utils::datetime::TimeOfDay
    // to be strictly less than 24h), so we prefer to fail on such values
    if (msg.hours() == 24) {
        ctx.AddError("'google.type.TimeOfDay' contains value that will overflow 'userver::utils::datetime::TimeOfDay'");
        return utils::datetime::TimeOfDay<TDuration>{};
    }

    if (msg.seconds() == 60) {
        ctx.AddError(
            "leap seconds are not supported when converting 'google.type.TimeOfDay' to "
            "'userver::utils::datetime::TimeOfDay'"
        );
        return utils::datetime::TimeOfDay<TDuration>{};
    }

    try {
        return TimeOfDay(utils::impl::InternalTag{}, msg.hours(), msg.minutes(), msg.seconds(), msg.nanos())
            .ToUserverTimeOfDay<TDuration>();
    } catch (const ValueError& e) {
        ctx.AddError(e.what());
        return utils::datetime::TimeOfDay<TDuration>{};
    }
}

template <typename TDuration>
void WriteProtoStruct(WriteContext&, const utils::datetime::TimeOfDay<TDuration>& obj, ::google::type::TimeOfDay& msg) {
    TimeOfDay time_of_day(obj);
    msg.set_hours(static_cast<int32_t>(time_of_day.Hours().count()));
    msg.set_minutes(static_cast<int32_t>(time_of_day.Minutes().count()));
    msg.set_seconds(static_cast<int32_t>(time_of_day.Seconds().count()));
    msg.set_nanos(static_cast<int32_t>(time_of_day.Nanos().count()));
}

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
