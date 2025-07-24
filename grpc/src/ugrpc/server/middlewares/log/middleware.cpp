#include <ugrpc/server/middlewares/log/middleware.hpp>

#include <userver/logging/log_extra.hpp>
#include <userver/tracing/tags.hpp>

#include <userver/ugrpc/status_codes.hpp>

#include <ugrpc/impl/logging.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::log {

namespace {

std::string GetMessageForLogging(const google::protobuf::Message& message, const Settings& settings) {
    if (!logging::ShouldLog(settings.msg_log_level)) {
        return "";
    }
    return ugrpc::impl::GetMessageForLogging(message, settings.max_msg_size);
}

class Logger {
public:
    explicit Logger(logging::Level log_level) : log_level_threshold_(log_level) {}

    void Log(logging::Level level, std::string_view message, logging::LogExtra&& extra) const {
        if (level < log_level_threshold_) {
            return;
        }
        LOG(level) << message << std::move(extra);
    }

private:
    logging::Level log_level_threshold_;
};

}  // namespace

Middleware::Middleware(const Settings& settings) : settings_(settings) {}

void Middleware::OnCallStart(MiddlewareCallContext& context) const {
    auto& span = context.GetSpan();
    span.SetLocalLogLevel(settings_.local_log_level);

    span.AddTag(ugrpc::impl::kComponentTag, "server");
    span.AddTag("meta_type", std::string{context.GetCallName()});
    span.AddNonInheritableTag(tracing::kSpanKind, tracing::kSpanKindServer);

    if (context.IsClientStreaming()) {
        Logger{settings_.log_level}.Log(
            settings_.msg_log_level, "gRPC request stream started", logging::LogExtra{{"type", "request"}}
        );
    }
}

void Middleware::PostRecvMessage(MiddlewareCallContext& context, google::protobuf::Message& request) const {
    const Logger logger{settings_.log_level};
    logging::LogExtra extra{
        {ugrpc::impl::kTypeTag, "request"},
        {ugrpc::impl::kBodyTag, GetMessageForLogging(request, settings_)},
        {ugrpc::impl::kMessageMarshalledLenTag, request.ByteSizeLong()},
    };
    if (context.IsClientStreaming()) {
        logger.Log(settings_.msg_log_level, "gRPC request stream message", std::move(extra));
    } else {
        extra.Extend("type", "request");
        logger.Log(settings_.msg_log_level, "gRPC request", std::move(extra));
    }
}

void Middleware::PreSendMessage(MiddlewareCallContext& context, google::protobuf::Message& response) const {
    const Logger logger{settings_.log_level};
    logging::LogExtra extra{
        {ugrpc::impl::kTypeTag, "response"},
        {"grpc_code", "OK"},  // TODO: revert
        {ugrpc::impl::kBodyTag, GetMessageForLogging(response, settings_)},
    };
    if (context.IsServerStreaming()) {
        logger.Log(settings_.msg_log_level, "gRPC response stream message", std::move(extra));
    } else {
        extra.Extend("type", "response");
        logger.Log(settings_.msg_log_level, "gRPC response", std::move(extra));
    }
}

void Middleware::OnCallFinish(MiddlewareCallContext& context, const grpc::Status& status) const {
    const Logger logger{settings_.log_level};
    if (status.ok()) {
        if (context.IsServerStreaming()) {
            logger.Log(
                settings_.msg_log_level, "gRPC response stream finished", logging::LogExtra{{"type", "response"}}
            );
        }
    } else {
        auto error_details = ugrpc::impl::GetErrorDetailsForLogging(status);
        logging::LogExtra extra{
            {"type", "response"},
            {ugrpc::impl::kCodeTag, ugrpc::ToString(status.error_code())},
            {ugrpc::impl::kTypeTag, "error_status"},
            {ugrpc::impl::kBodyTag, std::move(error_details)},
        };
        const auto error_log_level =
            IsServerError(status.error_code()) ? logging::Level::kError : logging::Level::kWarning;
        logger.Log(error_log_level, "gRPC error", std::move(extra));
    }
}

}  // namespace ugrpc::server::middlewares::log

USERVER_NAMESPACE_END
