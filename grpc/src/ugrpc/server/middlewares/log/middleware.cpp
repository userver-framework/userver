#include "middleware.hpp"

#include <userver/logging/level_serialization.hpp>
#include <userver/logging/log_extra.hpp>
#include <userver/tracing/span.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <ugrpc/impl/logging.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::log {

namespace {

bool IsRequestStream(CallKind kind) {
    return kind == CallKind::kRequestStream || kind == CallKind::kBidirectionalStream;
}

bool IsResponseStream(CallKind kind) {
    return kind == CallKind::kResponseStream || kind == CallKind::kBidirectionalStream;
}

std::string GetMessageForLogging(const google::protobuf::Message& message, const Settings& settings) {
    return ugrpc::impl::GetMessageForLogging(
        message,
        ugrpc::impl::MessageLoggingOptions{settings.msg_log_level, settings.max_msg_size, settings.trim_secrets}
    );
}

}  // namespace

Middleware::Middleware(const Settings& settings) : settings_(settings) {}

void Middleware::CallRequestHook(const MiddlewareCallContext& context, google::protobuf::Message& request) {
    auto& storage = context.GetCall().GetStorageContext();
    auto& span = context.GetCall().GetSpan();
    logging::LogExtra log_extra{{"grpc_type", "request"}, {"body", GetMessageForLogging(request, settings_)}};

    if (storage.Get(kIsFirstRequest)) {
        storage.Set(kIsFirstRequest, false);

        const auto call_kind = context.GetCall().GetCallKind();
        if (!IsRequestStream(call_kind)) {
            log_extra.Extend("type", "request");
        }
    }
    LOG(span.GetLogLevel()) << "gRPC request message" << std::move(log_extra);
}

void Middleware::CallResponseHook(const MiddlewareCallContext& context, google::protobuf::Message& response) {
    auto& span = context.GetCall().GetSpan();
    const auto call_kind = context.GetCall().GetCallKind();

    if (!IsResponseStream(call_kind)) {
        span.AddTag("grpc_type", "response");
        span.AddNonInheritableTag("body", GetMessageForLogging(response, settings_));
    } else {
        logging::LogExtra log_extra{{"grpc_type", "response"}, {"body", GetMessageForLogging(response, settings_)}};
        LOG(span.GetLogLevel()) << "gRPC response message" << std::move(log_extra);
    }
}

void Middleware::Handle(MiddlewareCallContext& context) const {
    auto& storage = context.GetCall().GetStorageContext();
    const auto call_kind = context.GetCall().GetCallKind();
    storage.Emplace(kIsFirstRequest, true);

    auto& span = context.GetCall().GetSpan();
    if (settings_.local_log_level) {
        span.SetLocalLogLevel(settings_.local_log_level);
    }

    span.AddTag("meta_type", std::string{context.GetCall().GetCallName()});
    span.AddNonInheritableTag("type", "response");
    if (IsResponseStream(call_kind)) {
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

    if (IsRequestStream(call_kind)) {
        // Just like in HTTP, there must be a single initial log
        // with type=request and some body. We don't have a real single request
        // (requests are written separately, 1 log per request), so we fake
        // the required request log.
        LOG(span.GetLogLevel()) << "gRPC request stream"
                                << logging::LogExtra{{"body", "request stream started"}, {"type", "request"}};
    }

    context.Next();
}

}  // namespace ugrpc::server::middlewares::log

USERVER_NAMESPACE_END
