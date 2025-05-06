#include <userver/ugrpc/server/impl/call_processor.hpp>

#include <chrono>

#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/opentelemetry.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/algo.hpp>

#include <ugrpc/impl/rpc_metadata.hpp>
#include <ugrpc/server/impl/format_log_message.hpp>
#include <userver/ugrpc/impl/statistics_scope.hpp>
#include <userver/ugrpc/impl/to_string.hpp>
#include <userver/ugrpc/server/impl/error_code.hpp>
#include <userver/ugrpc/status_codes.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

namespace {

void ReportFinishSuccess(
    const grpc::Status& status,
    tracing::Span& span,
    ugrpc::impl::RpcStatisticsScope& statistics_scope
) noexcept {
    try {
        const auto status_code = status.error_code();
        statistics_scope.OnExplicitFinish(status_code);

        span.AddTag("grpc_code", ugrpc::ToString(status_code));
        if (!status.ok()) {
            span.AddTag(tracing::kErrorFlag, true);
            span.AddTag(tracing::kErrorMessage, status.error_message());
            // TODO if Finish with error status fails, should we leave span level as INFO??
            //  Because that's what happens if ReportFinishSuccess is not called.
            span.SetLogLevel(IsServerError(status_code) ? logging::Level::kError : logging::Level::kWarning);
        }
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error in ReportFinishSuccess: " << ex;
    }
}

}  // namespace

void SetupSpan(
    std::optional<tracing::InPlaceSpan>& span_holder,
    grpc::ServerContext& context,
    std::string_view call_name
) {
    auto span_name = utils::StrCat("grpc/", call_name);
    const auto& client_metadata = context.client_metadata();

    const auto* const trace_id = utils::FindOrNullptr(client_metadata, ugrpc::impl::kXYaTraceId);
    const auto* const parent_span_id = utils::FindOrNullptr(client_metadata, ugrpc::impl::kXYaSpanId);
    const auto* const traceparent = utils::FindOrNullptr(client_metadata, ugrpc::impl::kTraceParent);
    if (traceparent) {
        auto extraction_result = tracing::opentelemetry::ExtractTraceParentData(ugrpc::impl::ToString(*traceparent));
        if (!extraction_result.has_value()) {
            LOG_LIMITED_WARNING() << fmt::format(
                "Invalid traceparent header format ({}). Skipping Opentelemetry "
                "headers",
                extraction_result.error()
            );
            span_holder.emplace(std::move(span_name), utils::impl::SourceLocation::Current());
        } else {
            auto data = std::move(extraction_result).value();
            span_holder.emplace(
                std::move(span_name),
                std::move(data.trace_id),
                std::move(data.span_id),
                utils::impl::SourceLocation::Current()
            );
        }
    } else if (trace_id) {
        span_holder.emplace(
            std::move(span_name),
            ugrpc::impl::ToString(*trace_id),
            parent_span_id ? ugrpc::impl::ToString(*parent_span_id) : std::string{},
            utils::impl::SourceLocation::Current()
        );
    } else {
        span_holder.emplace(std::move(span_name), utils::impl::SourceLocation::Current());
    }

    auto& span = span_holder->Get();

    const auto* const parent_link = utils::FindOrNullptr(client_metadata, ugrpc::impl::kXYaRequestId);
    if (parent_link) {
        span.SetParentLink(ugrpc::impl::ToString(*parent_link));
    }
}

grpc::Status ReportHandlerError(const std::exception& ex, CallState& state) noexcept {
    try {
        auto& span = state.GetSpan();
        span.AddNonInheritableTag(tracing::kErrorFlag, true);
        LOG_ERROR() << "Uncaught exception in '" << state.call_name << "': " << ex;
        span.AddTag(tracing::kErrorMessage, ex.what());
        span.SetLogLevel(logging::Level::kError);
        return kUnknownErrorStatus;
    } catch (const std::exception& new_ex) {
        LOG_ERROR() << "Error in ReportHandlerError: " << new_ex;
        return grpc::Status{grpc::StatusCode::INTERNAL, ""};
    }
}

grpc::Status ReportRpcInterruptedError(CallState& state) noexcept {
    try {
        // RPC interruption leads to asynchronous task cancellation by RpcFinishedEvent,
        // so the task either is already cancelled, or is going to be cancelled.
        LOG_WARNING() << "RPC interrupted in '" << state.call_name
                      << "'. The previously logged cancellation or network exception, if any, is likely caused by it.";
        state.statistics_scope.OnNetworkError();
        auto& span = state.GetSpan();
        span.AddTag(tracing::kErrorMessage, "RPC interrupted");
        span.AddTag(tracing::kErrorFlag, true);
        span.SetLogLevel(logging::Level::kWarning);
        return grpc::Status::CANCELLED;
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error in ReportRpcInterruptedError: " << ex;
        return grpc::Status{grpc::StatusCode::INTERNAL, ""};
    }
}

grpc::Status
ReportCustomError(const USERVER_NAMESPACE::server::handlers::CustomHandlerException& ex, CallState& state) noexcept {
    try {
        grpc::Status status{CustomStatusToGrpc(ex.GetCode()), ugrpc::impl::ToGrpcString(ex.GetExternalErrorBody())};

        const auto log_level = IsServerError(status.error_code()) ? logging::Level::kError : logging::Level::kWarning;
        LOG(log_level) << "Error in " << state.call_name << ": " << ex;
        auto& span = state.GetSpan();
        span.AddTag(tracing::kErrorFlag, true);
        span.AddTag(tracing::kErrorMessage, ex.what());
        span.SetLogLevel(log_level);
        return status;
    } catch (const std::exception& new_ex) {
        LOG_ERROR() << "Error in ReportCustomError: " << new_ex;
        return grpc::Status{grpc::StatusCode::INTERNAL, ""};
    }
}

void WriteAccessLog(
    MiddlewareCallContext& context,
    const grpc::Status& status,
    logging::TextLoggerRef access_tskv_logger
) noexcept {
    try {
        const auto& server_context = context.GetServerContext();
        constexpr auto kLevel = logging::Level::kInfo;

        if (access_tskv_logger.ShouldLog(kLevel)) {
            logging::impl::TextLogItem log_item{FormatLogMessage(
                server_context.client_metadata(),
                server_context.peer(),
                context.GetSpan().GetStartSystemTime(),
                context.GetCallName(),
                status.error_code()
            )};
            access_tskv_logger.Log(kLevel, log_item);
        }
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error in WriteAccessLog: " << ex;
    }
}

void CheckFinishStatus(bool finish_op_succeeded, const grpc::Status& status, CallState& state) noexcept {
    if (finish_op_succeeded) {
        ReportFinishSuccess(status, state.GetSpan(), state.statistics_scope);
    } else {
        ReportRpcInterruptedError(state);
    }
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
