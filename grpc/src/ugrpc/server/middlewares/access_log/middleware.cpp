#include <ugrpc/server/middlewares/access_log/middleware.hpp>

#include <userver/logging/level.hpp>

#include <ugrpc/server/impl/format_log_message.hpp>
#include <userver/logging/impl/logger_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::access_log {

namespace {

void WriteAccessLog(
    MiddlewareCallContext& context,
    const std::optional<grpc::Status>& status,
    logging::TextLoggerRef access_tskv_logger
) noexcept {
    try {
        const auto& server_context = context.GetServerContext();
        constexpr auto kLevel = logging::Level::kInfo;

        if (access_tskv_logger.ShouldLog(kLevel)) {
            const auto* log_extra = context.GetStorageContext().GetOptional(kLogExtraTag);

            access_tskv_logger.Log(
                kLevel,
                impl::FormatLogMessage(
                    server_context.client_metadata(),
                    server_context.peer(),
                    context.GetSpan().GetStartSystemTime(),
                    context.GetCallName(),
                    status.has_value() ? std::make_optional(status->error_code()) : std::nullopt,
                    log_extra
                )
                    .ExtractTextLogItem()
            );
        }
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error in WriteAccessLog: " << ex;
    }
}

}  // namespace

Middleware::Middleware(Settings&& settings)
    : logger_(std::move(settings.access_tskv_logger))
{}

void Middleware::OnCallFinish(MiddlewareCallContext& context, const std::optional<grpc::Status>& status) const {
    WriteAccessLog(context, status, *logger_);
}

}  // namespace ugrpc::server::middlewares::access_log

USERVER_NAMESPACE_END
