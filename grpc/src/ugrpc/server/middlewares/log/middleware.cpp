#include "middleware.hpp"

#include <userver/logging/level_serialization.hpp>
#include <userver/logging/log_extra.hpp>
#include <userver/tracing/span.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <ugrpc/impl/protobuf_utils.hpp>

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
    if (logging::ShouldLog(settings.msg_log_level)) {
        if (settings.trim_secrets) {
            return ugrpc::impl::ToJsonLogString(message, settings.max_msg_size);
        } else {
            return ugrpc::impl::ToJsonString(message, settings.max_msg_size);
        }
    } else {
        return "hidden by log level";
    }
}

}  // namespace

Settings Parse(const yaml_config::YamlConfig& config, formats::parse::To<Settings>) {
    Settings settings;
    settings.max_msg_size = config["msg-size-log-limit"].As<std::size_t>(settings.max_msg_size);
    settings.msg_log_level = config["msg-log-level"].As<logging::Level>(settings.msg_log_level);
    settings.local_log_level = config["log-level"].As<std::optional<logging::Level>>();
    settings.trim_secrets = config["trim-secrets"].As<bool>(settings.trim_secrets);
    return settings;
}

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
