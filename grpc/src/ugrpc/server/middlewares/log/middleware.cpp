#include <ugrpc/server/middlewares/log/middleware.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <userver/logging/log_extra.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/algo.hpp>

#include <ugrpc/impl/logging.hpp>
#include <ugrpc/impl/rpc_metadata.hpp>
#include <userver/ugrpc/protobuf_logging.hpp>
#include <userver/ugrpc/server/metadata_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::log {

namespace {

std::string GetMessageForLogging(const google::protobuf::Message& message, const Settings& settings) {
    if (!logging::ShouldLog(settings.msg_log_level)) {
        return "";
    }
    return ugrpc::ToLimitedDebugString(message, settings.max_msg_size);
}

class Logger {
public:
    explicit Logger(logging::Level log_level)
        : log_level_threshold_(log_level)
    {}

    void Log(logging::Level level, std::string_view message, logging::LogExtra&& extra) const {
        if (level < log_level_threshold_) {
            return;
        }
        LOG(level) << message << std::move(extra);
    }

private:
    logging::Level log_level_threshold_;
};

void AppendOriginMetadata(const CallContextBase& context, logging::LogExtra& extra) {
    const auto origin_values = GetRepeatedMetadata(context, ugrpc::impl::ToStringView(ugrpc::impl::kXOrigin));

    // TODO use std::ranges::empty in C++20.
    if (origin_values.begin() == origin_values.end()) {
        return;
    }

    extra.Extend(tracing::kUserAgent, fmt::to_string(fmt::join(origin_values, ";")));
}

}  // namespace

Middleware::Middleware(const Settings& settings)
    : settings_(settings)
{}

void Middleware::OnCallStart(MiddlewareCallContext& context) const {
    auto& span = context.GetSpan();
    span.SetLocalLogLevel(settings_.local_log_level);

    span.AddTag(ugrpc::impl::kComponentTag, "server");
    span.AddTag("meta_type", std::string{context.GetCallName()});

    const Logger logger{settings_.log_level};
    if (!IsSingleRequestMethod(context.GetRpcType())) {
        logging::LogExtra extra{{"type", "request"}};
        AppendOriginMetadata(context, extra);
        logger.Log(settings_.msg_log_level, "gRPC request stream started", std::move(extra));
    }
}

void Middleware::PostRecvMessage(MiddlewareCallContext& context, google::protobuf::Message& request) const {
    const Logger logger{settings_.log_level};
    logging::LogExtra extra{
        {ugrpc::impl::kTypeTag, "request"},
        {ugrpc::impl::kBodyTag, GetMessageForLogging(request, settings_)},
        {ugrpc::impl::kMessageMarshalledLenTag, request.ByteSizeLong()},
    };
    if (IsSingleRequestMethod(context.GetRpcType())) {
        extra.Extend("type", "request");
        AppendOriginMetadata(context, extra);
        logger.Log(settings_.msg_log_level, "gRPC request", std::move(extra));
    } else {
        logger.Log(settings_.msg_log_level, "gRPC request stream message", std::move(extra));
    }
}

void Middleware::PreSendMessage(MiddlewareCallContext& context, google::protobuf::Message& response) const {
    const Logger logger{settings_.log_level};
    logging::LogExtra extra{
        {ugrpc::impl::kTypeTag, "response"},
        {"grpc_code", "OK"},  // TODO: revert
        {ugrpc::impl::kBodyTag, GetMessageForLogging(response, settings_)},
        {ugrpc::impl::kMessageMarshalledLenTag, response.ByteSizeLong()},
    };
    if (IsSingleResponseMethod(context.GetRpcType())) {
        extra.Extend("type", "response");
        logger.Log(settings_.msg_log_level, "gRPC response", std::move(extra));
    } else {
        logger.Log(settings_.msg_log_level, "gRPC response stream message", std::move(extra));
    }
}

void Middleware::OnCallFinish(MiddlewareCallContext& context, const std::optional<grpc::Status>& status) const {
    const Logger logger{settings_.log_level};
    logging::LogExtra extra{{"type", "response"}};
    if (status.has_value()) {
        if (status->ok()) {
            if (!IsSingleResponseMethod(context.GetRpcType())) {
                logger.Log(settings_.msg_log_level, "gRPC response stream finished", std::move(extra));
            }
        } else {
            auto error_details = ugrpc::ToLimitedDebugString(*status, settings_.max_msg_size);
            extra.Extend({
                {ugrpc::impl::kCodeTag, ugrpc::ToString(status->error_code())},
                {ugrpc::impl::kTypeTag, "error_status"},
                {ugrpc::impl::kBodyTag, std::move(error_details)},
            });
            const auto default_error_log_level =
                IsServerError(status->error_code()) ? logging::Level::kError : logging::Level::kWarning;
            const auto error_log_level =
                utils::FindOrDefault(settings_.status_codes_log_level, status->error_code(), default_error_log_level);
            logger.Log(error_log_level, "gRPC error", std::move(extra));
        }
    } else {
        extra.Extend({
            {ugrpc::impl::kTypeTag, "error_status"},
            {ugrpc::impl::kBodyTag, "RPC interrupted"},
        });
        logger.Log(logging::Level::kWarning, "gRPC error", std::move(extra));
    }
}

}  // namespace ugrpc::server::middlewares::log

USERVER_NAMESPACE_END
