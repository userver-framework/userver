#include <userver/proto-structs/io/userver/proto_structs/timestamp_conv.hpp>

#include <google/protobuf/timestamp.pb.h>

#include <userver/proto-structs/exceptions.hpp>
#include <userver/proto-structs/io/context.hpp>
#include <userver/utils/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

Timestamp ReadProtoStruct(ReadContext& ctx, To<Timestamp>, const ::google::protobuf::Timestamp& msg) {
    try {
        return Timestamp(utils::impl::InternalTag{}, msg.seconds(), msg.nanos());
    } catch (const ValueError& e) {
        ctx.AddError(e.what());
        return Timestamp{};
    }
}

void WriteProtoStruct(WriteContext&, const Timestamp& obj, ::google::protobuf::Timestamp& msg) {
    msg.set_seconds(obj.Seconds().count());
    msg.set_nanos(static_cast<std::int32_t>(obj.Nanos().count()));
}

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
