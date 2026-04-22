#include <userver/proto-structs/io/std/chrono/time_point_conv.hpp>

#include <google/protobuf/timestamp.pb.h>

#include <userver/proto-structs/exceptions.hpp>
#include <userver/proto-structs/io/context.hpp>
#include <userver/proto-structs/timestamp.hpp>
#include <userver/utils/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

std::chrono::time_point<std::chrono::system_clock> ReadProtoStruct(
    ReadContext& ctx,
    To<std::chrono::time_point<std::chrono::system_clock>>,
    const ::google::protobuf::Timestamp& msg
) {
    try {
        return Timestamp(utils::impl::InternalTag{}, msg.seconds(), msg.nanos()).ToTimePoint();
    } catch (const ValueError& e) {
        ctx.AddError(e.what());
        return std::chrono::time_point<std::chrono::system_clock>{};
    }
}

void WriteProtoStruct(
    WriteContext& ctx,
    const std::chrono::time_point<std::chrono::system_clock>& obj,
    ::google::protobuf::Timestamp& msg
) {
    try {
        Timestamp ts{obj};
        msg.set_seconds(ts.Seconds().count());
        msg.set_nanos(static_cast<std::int32_t>(ts.Nanos().count()));
    } catch (const ValueError& e) {
        ctx.AddError(e.what());
    }
}

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
