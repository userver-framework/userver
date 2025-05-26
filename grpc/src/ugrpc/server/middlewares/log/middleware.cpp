#include <ugrpc/server/middlewares/log/middleware.hpp>

#include <userver/logging/log_extra.hpp>
#include <userver/tracing/tags.hpp>

#include <userver/ugrpc/status_codes.hpp>

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

void Middleware::OnCallStart(MiddlewareCallContext& context) const {
    auto& span = context.GetSpan();
    span.SetLocalLogLevel(settings_.local_log_level);

    span.AddTag(ugrpc::impl::kComponentTag, "server");
    span.AddTag("meta_type", std::string{context.GetCallName()});
    span.AddNonInheritableTag(tracing::kSpanKind, tracing::kSpanKindServer);

    if (context.IsClientStreaming()) {
        LOG_INFO() << "gRPC request stream started" << logging::LogExtra{{"type", "request"}};
    }
}

void Middleware::PostRecvMessage(MiddlewareCallContext& context, google::protobuf::Message& request) const {
    logging::LogExtra extra{
        {ugrpc::impl::kTypeTag, "request"},                  //
        {"body", GetMessageForLogging(request, settings_)},  //
    };
    if (context.IsClientStreaming()) {
        LOG_INFO() << "gRPC request stream message" << std::move(extra);
    } else {
        extra.Extend("type", "request");
        LOG_INFO() << "gRPC request" << std::move(extra);
    }
}

void Middleware::PreSendMessage(MiddlewareCallContext& context, google::protobuf::Message& response) const {
    logging::LogExtra extra{
        {ugrpc::impl::kTypeTag, "response"},                  //
        {"grpc_code", "OK"},                                  // TODO: revert
        {"body", GetMessageForLogging(response, settings_)},  //
    };
    if (context.IsServerStreaming()) {
        LOG_INFO() << "gRPC response stream message" << std::move(extra);
    } else {
        extra.Extend("type", "response");
        LOG_INFO() << "gRPC response" << std::move(extra);
    }
}

void Middleware::OnCallFinish(MiddlewareCallContext& context, const grpc::Status& status) const {
    if (status.ok()) {
        if (context.IsServerStreaming()) {
            LOG_INFO() << "gRPC response stream finished" << logging::LogExtra{{"type", "response"}};
        }
    } else {
        const auto log_level = IsServerError(status.error_code()) ? logging::Level::kError : logging::Level::kWarning;
        auto error_details = ugrpc::impl::GetErrorDetailsForLogging(status);
        logging::LogExtra extra{
            {"type", "response"},
            {ugrpc::impl::kTypeTag, "error_status"},
            {"body", std::move(error_details)},
        };

        LOG(log_level) << "gRPC error" << std::move(extra);
    }
}

}  // namespace ugrpc::server::middlewares::log

USERVER_NAMESPACE_END
