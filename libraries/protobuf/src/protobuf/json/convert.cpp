#include <userver/protobuf/json/convert.hpp>

#include <protobuf/json/impl/read.hpp>
#include <protobuf/json/impl/write.hpp>

USERVER_NAMESPACE_BEGIN

namespace protobuf::json {

formats::json::ValueBuilder MessageToJsonBuilder(
    const ::google::protobuf::Message& message,
    const PrintOptions& options
) {
    return impl::WriteMessage(message, options);
}

void JsonToMessage(
    const formats::json::Value& json,
    ::google::protobuf::Message& message,
    const ParseOptions& options
) {
    impl::ReadMessage(json, message, options);
}

}  // namespace protobuf::json

/*
namespace formats::serialize {

json::Value Serialize(const ::google::protobuf::Message& message, To<json::Value>) {
    return protobuf::json::MessageToJson(message);
}

}  // namespace formats::serialize
*/

USERVER_NAMESPACE_END
