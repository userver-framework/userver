#include <userver/ugrpc/proto_json.hpp>

#include <grpcpp/support/config.h>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

namespace {

const google::protobuf::util::JsonPrintOptions kOptions = []() {
  google::protobuf::util::JsonPrintOptions options;
  options.always_print_primitive_fields = true;
  return options;
}();
}

formats::json::Value MessageToJson(const google::protobuf::Message& message) {
  return formats::json::FromString(ToJsonString(message));
}

std::string ToString(const google::protobuf::Message& message) {
  return message.DebugString();
}

std::string ToJsonString(const google::protobuf::Message& message) {
  grpc::string result{};

  auto status =
      google::protobuf::util::MessageToJsonString(message, &result, kOptions);

  if (!status.ok()) {
    throw formats::json::ConversionException(
        "Cannot convert protobuf to string");
  }

  return result;
}

}  // namespace ugrpc

namespace formats::serialize {

json::Value Serialize(const google::protobuf::Message& message,
                      To<json::Value>) {
  return ugrpc::MessageToJson(message);
}

}  // namespace formats::serialize

USERVER_NAMESPACE_END
