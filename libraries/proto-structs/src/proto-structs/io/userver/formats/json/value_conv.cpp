#include <userver/proto-structs/io/userver/formats/json/value_conv.hpp>

#include <google/protobuf/struct.pb.h>

#include <userver/proto-structs/io/context.hpp>
#include <userver/protobuf/json/convert.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

formats::json::Value ReadProtoStruct(ReadContext& ctx, To<formats::json::Value>, const ::google::protobuf::Value& msg) {
    try {
        return protobuf::json::MessageToJson(msg, protobuf::json::PrintOptions{});
    } catch (const protobuf::json::PrintError& e) {
        ctx.AddError(e.what());
        return formats::json::Value{};
    }
}

void WriteProtoStruct(WriteContext&, const formats::json::Value& obj, ::google::protobuf::Value& msg) {
    protobuf::json::JsonToMessage(obj, msg, protobuf::json::ParseOptions{});
}

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
