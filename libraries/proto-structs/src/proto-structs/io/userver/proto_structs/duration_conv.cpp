#include <userver/proto-structs/io/userver/proto_structs/duration_conv.hpp>

#include <google/protobuf/duration.pb.h>

#include <userver/proto-structs/exceptions.hpp>
#include <userver/proto-structs/io/context.hpp>
#include <userver/utils/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

Duration ReadProtoStruct(ReadContext& ctx, To<Duration>, const ::google::protobuf::Duration& msg) {
    try {
        return Duration(utils::impl::InternalTag{}, msg.seconds(), msg.nanos());
    } catch (const ValueError& e) {
        ctx.AddError(e.what());
        return Duration{};
    }
}

void WriteProtoStruct(WriteContext&, const Duration& obj, ::google::protobuf::Duration& msg) {
    msg.set_seconds(obj.Seconds().count());
    msg.set_nanos(static_cast<std::int32_t>(obj.Nanos().count()));
}

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
