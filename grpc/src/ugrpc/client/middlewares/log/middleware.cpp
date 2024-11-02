#include "middleware.hpp"

#include <userver/logging/log_extra.hpp>

#include <ugrpc/impl/logging.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::log {

namespace {

bool IsSingleRequest(CallKind kind) { return CallKind::kUnaryCall == kind || CallKind::kInputStream == kind; }

bool IsSingleResponse(CallKind kind) { return CallKind::kUnaryCall == kind || CallKind::kOutputStream == kind; }

std::string GetMessageForLogging(const google::protobuf::Message& message, const Settings& settings) {
    return ugrpc::impl::GetMessageForLogging(
        message,
        ugrpc::impl::MessageLoggingOptions{settings.msg_log_level, settings.max_msg_size, settings.trim_secrets}
    );
}

class SpanLogger {
public:
    SpanLogger(const tracing::Span& span, logging::Level log_level) : span_{span}, log_level_{log_level} {}

    void Log(std::string_view message, logging::LogExtra&& extra) const {
        const tracing::impl::DetachLocalSpansScope ignore_local_span;
        LOG(log_level_) << message << std::move(extra) << tracing::impl::LogSpanAsLastNoCurrent{span_};
    }

private:
    const tracing::Span& span_;
    logging::Level log_level_{};
};

}  // namespace

Middleware::Middleware(const Settings& settings) : settings_(settings) {}

void Middleware::PreStartCall(MiddlewareCallContext& context) const {
    auto& span = context.GetSpan();
    span.SetLocalLogLevel(settings_.log_level);

    span.AddTag("meta_type", std::string{context.GetCallName()});

    if (!IsSingleRequest(context.GetCallKind())) {
        SpanLogger{context.GetSpan(), settings_.log_level}.Log(
            "gRPC request stream started", logging::LogExtra{{"grpc_type", "request"}}
        );
    }
}

void Middleware::PreSendMessage(MiddlewareCallContext& context, const google::protobuf::Message& message) const {
    SpanLogger logger{context.GetSpan(), settings_.log_level};
    logging::LogExtra extra{{"grpc_type", "request"}, {"body", GetMessageForLogging(message, settings_)}};
    if (IsSingleRequest(context.GetCallKind())) {
        logger.Log("gRPC request", std::move(extra));
    } else {
        logger.Log("gRPC request stream message", std::move(extra));
    }
}

void Middleware::PostRecvMessage(MiddlewareCallContext& context, const google::protobuf::Message& message) const {
    SpanLogger logger{context.GetSpan(), settings_.log_level};
    logging::LogExtra extra{{"grpc_type", "response"}, {"body", GetMessageForLogging(message, settings_)}};
    if (IsSingleResponse(context.GetCallKind())) {
        logger.Log("gRPC response", std::move(extra));
    } else {
        logger.Log("gRPC response stream message", std::move(extra));
    }
}

void Middleware::PostFinish(MiddlewareCallContext& context, const grpc::Status& /*status*/) const {
    if (!IsSingleResponse(context.GetCallKind())) {
        SpanLogger{context.GetSpan(), settings_.log_level}.Log(
            "gRPC response stream finished", logging::LogExtra{{"grpc_type", "response"}}
        );
    }
}

MiddlewareFactory::MiddlewareFactory(const Settings& settings) : settings_(settings) {}

std::shared_ptr<const MiddlewareBase> MiddlewareFactory::GetMiddleware(std::string_view /*client_name*/) const {
    return std::make_shared<Middleware>(settings_);
}

}  // namespace ugrpc::client::middlewares::log

USERVER_NAMESPACE_END
