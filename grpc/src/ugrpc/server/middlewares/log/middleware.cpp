#include "middleware.hpp"

#include <userver/logging/log_extra.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::log {

Middleware::Middleware(const Settings& settings) : settings_(settings) {}

void Middleware::CallRequestHook(const MiddlewareCallContext& context,
                                 google::protobuf::Message& request) {
  if (settings_.local_log_level) {
    auto& span = tracing::Span::CurrentSpan();
    span.SetLocalLogLevel(*settings_.local_log_level);
  }

  const logging::LogExtra log_extra{
      {"grpc_service", std::string(context.GetServiceName())},
      {"grpc_method", std::string(context.GetMethodName())}};
  auto log_func = [&]() -> std::string {
    const auto& str = request.Utf8DebugString();
    return utils::log::ToLimitedUtf8(str, settings_.max_msg_size);
  };

  LOG(settings_.msg_log_level)
      << "gRPC request message: " << log_func() << log_extra;
}

void Middleware::CallResponseHook(const MiddlewareCallContext& context,
                                  google::protobuf::Message& response) {
  if (settings_.local_log_level) {
    auto& span = tracing::Span::CurrentSpan();
    span.SetLocalLogLevel(*settings_.local_log_level);
  }
  const logging::LogExtra log_extra{
      {"grpc_service", std::string(context.GetServiceName())},
      {"grpc_method", std::string(context.GetMethodName())}};
  auto log_func = [&]() -> std::string {
    const auto& str = response.Utf8DebugString();
    return utils::log::ToLimitedUtf8(str, settings_.max_msg_size);
  };
  LOG(settings_.msg_log_level) << "gRPC response message: " << log_func();
}

void Middleware::Handle(MiddlewareCallContext& context) const {
  if (settings_.local_log_level) {
    auto& span = tracing::Span::CurrentSpan();
    span.SetLocalLogLevel(*settings_.local_log_level);
  }

  context.Next();
}

}  // namespace ugrpc::server::middlewares::log

USERVER_NAMESPACE_END
