#include <ugrpc/server/middlewares/log/middleware.hpp>

#include <chrono>
#include <utility>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <userver/logging/log.hpp>
#include <userver/logging/log_extra.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/algo.hpp>

#include <ugrpc/impl/logging.hpp>
#include <ugrpc/impl/rpc_metadata.hpp>
#include <userver/ugrpc/protobuf_logging.hpp>
#include <userver/ugrpc/server/metadata_utils.hpp>
#include <userver/ugrpc/server/storage_context.hpp>
#include <userver/utils/any_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::log {

namespace {

const utils::AnyStorageDataTag<StorageContext, logging::LogExtra> kUnaryResponseLogExtraTag{};

std::string GetMessageForLogging(const google::protobuf::Message& message, const Settings& settings) {
    if (settings.msg_log_level < settings.log_level || !logging::ShouldLog(settings.msg_log_level)) {
        return "";
    }
    return ugrpc::ToLimitedLoggingString(message, settings.max_msg_size);
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

    template <typename LogBuilder>
    void Log(logging::Level level, LogBuilder&& log_builder) const {
        if (level < log_level_threshold_) {
            return;
        }
        LOG(level) << std::forward<LogBuilder>(log_builder);
    }

    bool ShouldLog(logging::Level level) const { return level >= log_level_threshold_ && logging::ShouldLog(level); }

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

void AppendDelay(MiddlewareCallContext& context, logging::LogExtra& extra) {
    const auto delay = std::chrono::duration_cast<
        std::chrono::microseconds>(std::chrono::steady_clock::now() - context.GetSpan().GetStartSteadyTime());
    const auto delay_s = std::chrono::duration_cast<std::chrono::seconds>(delay);
    const auto delay_us = delay - delay_s;
    extra.Extend("delay", fmt::format("{}.{:06}", delay_s.count(), delay_us.count()));
}

logging::LogExtra MakeResponseLogExtra(const google::protobuf::Message& response, const Settings& settings) {
    return {
        {ugrpc::impl::kTypeTag, "response"},
        {"grpc_code", "OK"},  // TODO: revert
        {ugrpc::impl::kBodyTag, GetMessageForLogging(response, settings)},
        {ugrpc::impl::kMessageMarshalledLenTag, response.ByteSizeLong()},
    };
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
    logger.Log(settings_.msg_log_level, [&](auto& log_helper) {
        logging::LogExtra extra{
            {ugrpc::impl::kTypeTag, "request"},
            {ugrpc::impl::kBodyTag, GetMessageForLogging(request, settings_)},
            {ugrpc::impl::kMessageMarshalledLenTag, request.ByteSizeLong()},
        };
        if (IsSingleRequestMethod(context.GetRpcType())) {
            extra.Extend("type", "request");
            AppendOriginMetadata(context, extra);
            log_helper << "gRPC request" << std::move(extra);
        } else {
            log_helper << "gRPC request stream message" << std::move(extra);
        }
    });
}

void Middleware::PreSendMessage(MiddlewareCallContext& context, google::protobuf::Message& response) const {
    const Logger logger{settings_.log_level};
    if (IsSingleResponseMethod(context.GetRpcType())) {
        if (logger.ShouldLog(settings_.msg_log_level)) {
            auto extra = MakeResponseLogExtra(response, settings_);
            extra.Extend("type", "response");
            context.GetStorageContext().Set(kUnaryResponseLogExtraTag, std::move(extra));
        }
    } else {
        logger.Log(settings_.msg_log_level, [&](auto& log_helper) {
            log_helper << "gRPC response stream message" << MakeResponseLogExtra(response, settings_);
        });
    }
}

void Middleware::OnCallFinish(MiddlewareCallContext& context, const std::optional<grpc::Status>& status) const {
    const Logger logger{settings_.log_level};
    logging::LogExtra extra{{"type", "response"}};
    if (status.has_value()) {
        if (status->ok()) {
            if (IsSingleResponseMethod(context.GetRpcType())) {
                auto* const response_extra = context.GetStorageContext().GetOptional(kUnaryResponseLogExtraTag);
                if (response_extra) {
                    logger.Log(settings_.msg_log_level, [&](auto& log_helper) {
                        AppendDelay(context, *response_extra);
                        log_helper << "gRPC response" << std::move(*response_extra);
                    });
                }
            } else {
                AppendDelay(context, extra);
                logger.Log(settings_.msg_log_level, "gRPC response stream finished", std::move(extra));
            }
        } else {
            auto error_details = ugrpc::ToLimitedLoggingString(*status, settings_.max_msg_size);
            extra.Extend({
                {ugrpc::impl::kCodeTag, std::string(ugrpc::ToStringView(status->error_code()))},
                {ugrpc::impl::kTypeTag, "error_status"},
                {ugrpc::impl::kBodyTag, std::move(error_details)},
            });
            const auto default_error_log_level =
                IsServerError(status->error_code()) ? logging::Level::kError : logging::Level::kWarning;
            const auto error_log_level =
                utils::FindOrDefault(settings_.status_codes_log_level, status->error_code(), default_error_log_level);
            AppendDelay(context, extra);
            logger.Log(error_log_level, "gRPC error", std::move(extra));
        }
    } else {
        extra.Extend({
            {ugrpc::impl::kTypeTag, "error_status"},
            {ugrpc::impl::kBodyTag,
             "Call is interrupted before it was finished and response with status code was sent (it is not a server "
             "error, most likely client cancelled the call because the result is not needed anymore)"},
        });
        AppendDelay(context, extra);
        logger.Log(logging::Level::kWarning, "gRPC error", std::move(extra));
    }
}

}  // namespace ugrpc::server::middlewares::log

USERVER_NAMESPACE_END
