#include "middleware.hpp"

#include <userver/logging/log_extra.hpp>
#include <userver/tracing/tags.hpp>

#include <ugrpc/impl/logging.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::log {

namespace {

std::string GetMessageForLogging(const google::protobuf::Message& message, const Settings& settings) {
    return ugrpc::impl::GetMessageForLogging(
        message,
        ugrpc::impl::MessageLoggingOptions{settings.msg_log_level, settings.max_msg_size, settings.trim_secrets}
    );
}

class SpanLogger {
public:
    explicit SpanLogger(const tracing::Span& span) : span_{span} {}

    void Log(std::string_view message, logging::LogExtra&& extra) const {
        const tracing::impl::DetachLocalSpansScope ignore_local_span;
        LOG_INFO() << message << std::move(extra) << tracing::impl::LogSpanAsLastNoCurrent{span_};
    }

private:
    const tracing::Span& span_;
};

}  // namespace

Middleware::Middleware(const Settings& settings) : settings_(settings) {}

void Middleware::PreStartCall(MiddlewareCallContext& context) const {
    auto& span = context.GetSpan();

    span.AddTag("meta_type", std::string{context.GetCallName()});
    span.AddTag(tracing::kSpanKind, tracing::kSpanKindClient);

    if (context.IsClientStreaming()) {
        SpanLogger{context.GetSpan()}.Log("gRPC request stream started", logging::LogExtra{{"grpc_type", "request"}});
    }
}

void Middleware::PreSendMessage(MiddlewareCallContext& context, const google::protobuf::Message& message) const {
    SpanLogger logger{context.GetSpan()};
    logging::LogExtra extra{{"grpc_type", "request"}, {"body", GetMessageForLogging(message, settings_)}};
    if (context.IsClientStreaming()) {
        logger.Log("gRPC request stream message", std::move(extra));
    } else {
        logger.Log("gRPC request", std::move(extra));
    }
}

void Middleware::PostRecvMessage(MiddlewareCallContext& context, const google::protobuf::Message& message) const {
    SpanLogger logger{context.GetSpan()};
    logging::LogExtra extra{{"grpc_type", "response"}, {"body", GetMessageForLogging(message, settings_)}};
    if (context.IsServerStreaming()) {
        logger.Log("gRPC response stream message", std::move(extra));
    } else {
        logger.Log("gRPC response", std::move(extra));
    }
}

void Middleware::PostFinish(MiddlewareCallContext& context, const grpc::Status& /*status*/) const {
    if (context.IsServerStreaming()) {
        SpanLogger{context.GetSpan()}.Log(
            "gRPC response stream finished", logging::LogExtra{{"grpc_type", "response"}}
        );
    }
}

}  // namespace ugrpc::client::middlewares::log

USERVER_NAMESPACE_END
