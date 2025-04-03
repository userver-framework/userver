#include "middleware.hpp"

#include <userver/logging/log_extra.hpp>
#include <userver/tracing/tags.hpp>

#include <ugrpc/impl/logging.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::log {

namespace {

std::string GetMessageForLogging(const google::protobuf::Message& message, const Settings& settings) {
    return ugrpc::impl::GetMessageForLogging(
        message,
        ugrpc::impl::MessageLoggingOptions{settings.msg_log_level, settings.max_msg_size, settings.trim_secrets}
    );
}

}  // namespace

Middleware::Middleware(const Settings& settings) : settings_(settings) {}

void Middleware::CallRequestHook(const MiddlewareCallContext& context, google::protobuf::Message& request) {
    logging::LogExtra extra{{"grpc_type", "request"}, {"body", GetMessageForLogging(request, settings_)}};
    if (!context.IsClientStreaming()) {
        extra.Extend("type", "request");
    }
    LOG_INFO() << "gRPC request message" << std::move(extra);
}

void Middleware::CallResponseHook(const MiddlewareCallContext& context, google::protobuf::Message& response) {
    if (!context.IsServerStreaming()) {
        auto& span = context.GetCall().GetSpan();
        span.AddTag("grpc_type", "response");
        span.AddNonInheritableTag("body", GetMessageForLogging(response, settings_));
    } else {
        LOG_INFO() << "gRPC response message"
                   << logging::LogExtra{{"grpc_type", "response"}, {"body", GetMessageForLogging(response, settings_)}};
    }
}

void Middleware::Handle(MiddlewareCallContext& context) const {
    auto& span = context.GetCall().GetSpan();

    span.AddTag("meta_type", std::string{context.GetCall().GetCallName()});
    span.AddNonInheritableTag("type", "response");
    span.AddNonInheritableTag(tracing::kSpanKind, tracing::kSpanKindServer);
    if (context.IsServerStreaming()) {
        // Just like in HTTP, there must be a single trailing Span log
        // with type=response and some `body`. We don't have a real single response
        // (responses are written separately, 1 log per response), so we fake
        // the required response log.
        span.AddNonInheritableTag("body", "response stream finished");
    } else {
        // Write this dummy `body` in case unary response RPC fails
        // (with or without status) before receiving the response.
        // If the RPC finishes with OK status, `body` tag will be overwritten.
        span.AddNonInheritableTag("body", "error status");
    }

    if (context.IsClientStreaming()) {
        // Just like in HTTP, there must be a single initial log
        // with type=request and some body. We don't have a real single request
        // (requests are written separately, 1 log per request), so we fake
        // the required request log.
        LOG_INFO() << "gRPC request stream"
                   << logging::LogExtra{{"body", "request stream started"}, {"type", "request"}};
    }

    context.Next();
}

}  // namespace ugrpc::server::middlewares::log

USERVER_NAMESPACE_END
