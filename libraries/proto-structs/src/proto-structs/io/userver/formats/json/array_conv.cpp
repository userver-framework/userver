#include <userver/proto-structs/io/userver/formats/json/array_conv.hpp>

#include <google/protobuf/struct.pb.h>

#include <userver/proto-structs/io/context.hpp>
#include <userver/protobuf/json/convert.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

formats::json::Array ReadProtoStruct(
    ReadContext& ctx,
    To<formats::json::Array>,
    const ::google::protobuf::ListValue& msg
) {
    try {
        return formats::json::Array{protobuf::json::MessageToJson(msg, protobuf::json::PrintOptions{})};
    } catch (const protobuf::json::PrintError& e) {
        ctx.AddError(e.what());
        return formats::json::Array{};
    }
}

void WriteProtoStruct(WriteContext&, const formats::json::Array& obj, ::google::protobuf::ListValue& msg) {
    protobuf::json::JsonToMessage(obj.GetValue(), msg, protobuf::json::ParseOptions{});
}

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
