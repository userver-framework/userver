#include "middleware.hpp"

#include <fmt/core.h>
#include <google/protobuf/util/json_util.h>

#include <userver/logging/log_extra.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::log {

namespace {

std::string ProtobufAsJsonString(const google::protobuf::Message& message,
                                 uint64_t max_msg_size) {
  google::protobuf::util::JsonPrintOptions options;
  options.add_whitespace = false;
  grpc::string as_json;
  const auto status =
      google::protobuf::util::MessageToJsonString(message, &as_json, options);
  if (!status.ok()) {
    std::string log =
        fmt::format("Error getting a json string: {}", status.message().data());
    LOG_WARNING() << log;
    return log;
  }
  return std::string{utils::log::ToLimitedUtf8(as_json, max_msg_size)};
};

}  // namespace

Middleware::Middleware(const Settings& settings) : settings_(settings) {}

void Middleware::CallRequestHook(const MiddlewareCallContext& context,
                                 google::protobuf::Message& request) {
  auto& storage = context.GetCall().GetStorageContext();
  auto log = ProtobufAsJsonString(request, settings_.max_msg_size);

  logging::LogExtra log_extra{{"grpc_type", "request"},
                              {"body", std::move(log)}};

  if (storage.Get(kIsFirstRequest)) {
    storage.Set(kIsFirstRequest, false);
    log_extra.Extend("type", "request");
  }
  LOG(settings_.msg_log_level)
      << "gRPC request message" << std::move(log_extra);
}

void Middleware::CallResponseHook(const MiddlewareCallContext& context,
                                  google::protobuf::Message& response) {
  auto& span = context.GetCall().GetSpan();
  const auto call_kind = context.GetCall().GetCallKind();
  auto log = ProtobufAsJsonString(response, settings_.max_msg_size);

  if (call_kind == CallKind::kUnaryCall ||
      call_kind == CallKind::kRequestStream) {
    span.AddTag("grpc_type", "response");
    span.AddNonInheritableTag("type", "response");
    if (logging::ShouldLog(settings_.msg_log_level)) {
      span.AddNonInheritableTag("body", std::move(log));
    } else {
      span.AddNonInheritableTag("body", "hidden by log level");
    }
  } else {
    logging::LogExtra log_extra{{"grpc_type", "response"},
                                {"body", std::move(log)}};
    LOG(settings_.msg_log_level)
        << "gRPC response message" << std::move(log_extra);
  }
}

void Middleware::Handle(MiddlewareCallContext& context) const {
  auto& storage = context.GetCall().GetStorageContext();
  const auto call_kind = context.GetCall().GetCallKind();
  storage.Emplace(kIsFirstRequest, true);

  auto& span = context.GetCall().GetSpan();
  if (settings_.local_log_level) {
    span.SetLocalLogLevel(*settings_.local_log_level);
  }
  const auto meta_type =
      fmt::format("{}/{}", context.GetServiceName(), context.GetMethodName());

  span.AddTag("meta_type", meta_type);
  if (call_kind == CallKind::kResponseStream ||
      call_kind == CallKind::kBidirectionalStream) {
    span.AddNonInheritableTag("type", "response");
    span.AddNonInheritableTag("body", "stream finished");
  }

  context.Next();
}

}  // namespace ugrpc::server::middlewares::log

USERVER_NAMESPACE_END
