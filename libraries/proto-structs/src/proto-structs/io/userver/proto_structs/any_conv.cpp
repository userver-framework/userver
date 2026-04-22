#include <userver/proto-structs/io/userver/proto_structs/any_conv.hpp>

#include <userver/proto-structs/any.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

proto_structs::Any ReadProtoStruct(ReadContext&, To<proto_structs::Any>, const ::google::protobuf::Any& msg) {
    return proto_structs::Any{msg};
}

void WriteProtoStruct(WriteContext&, const proto_structs::Any& obj, ::google::protobuf::Any& msg) {
    msg = obj.GetProtobufAny();
}

void WriteProtoStruct(WriteContext&, proto_structs::Any&& obj, ::google::protobuf::Any& msg) {
    msg = std::move(obj).GetProtobufAny();
}

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
