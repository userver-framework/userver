#include <userver/proto-structs/io/userver/formats/json/object_conv.hpp>

#include <google/protobuf/struct.pb.h>

#include <userver/proto-structs/io/context.hpp>
#include <userver/protobuf/json/convert.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

formats::json::Object ReadProtoStruct(
    ReadContext& ctx,
    To<formats::json::Object>,
    const ::google::protobuf::Struct& msg
) {
    try {
        return formats::json::Object{protobuf::json::MessageToJson(msg, protobuf::json::PrintOptions{})};
    } catch (const protobuf::json::PrintError& e) {
        ctx.AddError(e.what());
        return formats::json::Object{};
    }
}

void WriteProtoStruct(WriteContext&, const formats::json::Object& obj, ::google::protobuf::Struct& msg) {
    protobuf::json::JsonToMessage(obj.GetValue(), msg, protobuf::json::ParseOptions{});
}

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
