#include "middleware.hpp"

#include <userver/logging/level.hpp>
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

    void Log(logging::Level level, std::string_view message, logging::LogExtra&& extra) const {
        const tracing::impl::DetachLocalSpansScope ignore_local_span;
        LOG(level) << message << std::move(extra) << tracing::impl::LogSpanAsLastNoCurrent{span_};
    }

private:
    const tracing::Span& span_;
};

}  // namespace

Middleware::Middleware(const Settings& settings) : settings_(settings) {}

void Middleware::PreStartCall(MiddlewareCallContext& context) const {
    auto& span = context.GetSpan();

    span.AddTag(ugrpc::impl::kComponentTag, "client");
    span.AddTag("meta_type", std::string{context.GetCallName()});
    span.AddTag(tracing::kSpanKind, tracing::kSpanKindClient);

    if (context.IsClientStreaming()) {
        SpanLogger{context.GetSpan()}.Log(logging::Level::kInfo, "gRPC request stream started", logging::LogExtra{});
    }
}

/// [MiddlewareBase Message methods example]
void Middleware::PreSendMessage(MiddlewareCallContext& context, const google::protobuf::Message& message) const {
    SpanLogger logger{context.GetSpan()};
    logging::LogExtra extra{
        {ugrpc::impl::kTypeTag, "request"},                  //
        {"body", GetMessageForLogging(message, settings_)},  //
    };
    if (context.IsClientStreaming()) {
        logger.Log(logging::Level::kInfo, "gRPC request stream message", std::move(extra));
    } else {
        logger.Log(logging::Level::kInfo, "gRPC request", std::move(extra));
    }
}

void Middleware::PostRecvMessage(MiddlewareCallContext& context, const google::protobuf::Message& message) const {
    SpanLogger logger{context.GetSpan()};
    logging::LogExtra extra{
        {ugrpc::impl::kTypeTag, "response"},                 //
        {"body", GetMessageForLogging(message, settings_)},  //
    };
    if (context.IsServerStreaming()) {
        logger.Log(logging::Level::kInfo, "gRPC response stream message", std::move(extra));
    } else {
        logger.Log(logging::Level::kInfo, "gRPC response", std::move(extra));
    }
}
/// [MiddlewareBase Message methods example]

void Middleware::PostFinish(MiddlewareCallContext& context, const grpc::Status& status) const {
    SpanLogger logger{context.GetSpan()};
    if (status.ok()) {
        if (context.IsServerStreaming()) {
            SpanLogger{context.GetSpan()}.Log(
                logging::Level::kInfo, "gRPC response stream finished", logging::LogExtra{}
            );
        }
    } else {
        auto error_details = ugrpc::impl::GetErrorDetailsForLogging(status);
        logging::LogExtra extra{
            {ugrpc::impl::kTypeTag, "error_status"},            //
            {tracing::kErrorMessage, std::move(error_details)}  //
        };

        logger.Log(logging::Level::kWarning, "gRPC error", std::move(extra));
    }
}

}  // namespace ugrpc::client::middlewares::log

USERVER_NAMESPACE_END
