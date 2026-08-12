#include <userver/proto-structs/io/userver/proto_structs/time_of_day_conv.hpp>

#include <google/type/timeofday.pb.h>

#include <userver/proto-structs/exceptions.hpp>
#include <userver/proto-structs/io/context.hpp>
#include <userver/utils/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

TimeOfDay ReadProtoStruct(ReadContext& ctx, To<TimeOfDay>, const ::google::type::TimeOfDay& msg) {
    try {
        return TimeOfDay(utils::impl::InternalTag{}, msg.hours(), msg.minutes(), msg.seconds(), msg.nanos());
    } catch (const ValueError& e) {
        ctx.AddError(e.what());
        return TimeOfDay{};
    }
}

void WriteProtoStruct(WriteContext&, const TimeOfDay& obj, ::google::type::TimeOfDay& msg) {
    msg.set_hours(static_cast<std::int32_t>(obj.Hours().count()));
    msg.set_minutes(static_cast<std::int32_t>(obj.Minutes().count()));
    msg.set_seconds(static_cast<std::int32_t>(obj.Seconds().count()));
    msg.set_nanos(static_cast<std::int32_t>(obj.Nanos().count()));
}

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
