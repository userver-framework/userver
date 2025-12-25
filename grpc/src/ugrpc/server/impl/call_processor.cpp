#include <userver/ugrpc/server/impl/call_processor.hpp>

#include <userver/tracing/opentelemetry.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/algo.hpp>

#include <ugrpc/impl/rpc_metadata.hpp>
#include <userver/ugrpc/impl/statistics_scope.hpp>
#include <userver/ugrpc/impl/to_string.hpp>
#include <userver/ugrpc/server/impl/error_code.hpp>
#include <userver/ugrpc/status_codes.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

namespace {

logging::Level AdjustLogLevelForCancellations(logging::Level level) {
    return engine::current_task::ShouldCancel() ? std::min(level, logging::Level::kWarning) : level;
}

}  // namespace

void SetupSpan(
    std::optional<tracing::InPlaceSpan>& span_holder,
    grpc::ServerContext& context,
    std::string_view call_name,
    std::string_view service_name,
    std::string_view method_name
) {
    auto span_name = call_name;
    const auto& client_metadata = context.client_metadata();

    const auto* const traceparent = utils::FindOrNullptr(client_metadata, ugrpc::impl::kTraceParent);
    if (traceparent) {
        auto extraction_result =
            tracing::opentelemetry::ExtractTraceParentDataView(ugrpc::impl::ToStringView(*traceparent));
        if (!extraction_result.has_value()) {
            LOG_LIMITED_WARNING() << fmt::format(
                "Invalid traceparent header format ({}). Skipping Opentelemetry "
                "headers",
                extraction_result.error()
            );
            span_holder.emplace(std::string{span_name}, utils::impl::SourceLocation::Current());
        } else {
            auto data = std::move(extraction_result).value();
            span_holder
                .emplace(std::string{span_name}, data.trace_id, data.span_id, utils::impl::SourceLocation::Current());
        }
    } else if (const auto* const trace_id = utils::FindOrNullptr(client_metadata, ugrpc::impl::kXYaTraceId)) {
        const auto* const parent_span_id = utils::FindOrNullptr(client_metadata, ugrpc::impl::kXYaSpanId);
        span_holder.emplace(
            std::string{span_name},
            ugrpc::impl::ToStringView(*trace_id),
            parent_span_id ? ugrpc::impl::ToStringView(*parent_span_id) : std::string_view{},
            utils::impl::SourceLocation::Current()
        );
    } else {
        span_holder.emplace(std::string{span_name}, utils::impl::SourceLocation::Current());
    }

    auto& span = span_holder->Get();
    const auto* const parent_link = utils::FindOrNullptr(client_metadata, ugrpc::impl::kXYaRequestId);
    if (parent_link) {
        span.SetParentLink(ugrpc::impl::ToStringView(*parent_link));
    }

    span.AddNonInheritableTag(tracing::kSpanKind, tracing::kSpanKindServer);
    span.AddNonInheritableTag(tracing::kRpcSystem, "grpc");
    span.AddNonInheritableTag(tracing::kRpcService, std::string{service_name});
    span.AddNonInheritableTag(tracing::kRpcMethod, std::string{method_name});
}

grpc::Status ReportCustomError(const USERVER_NAMESPACE::server::handlers::CustomHandlerException& ex, CallState& state)
    noexcept {
    try {
        grpc::Status status{CustomStatusToGrpc(ex.GetCode()), ex.GetExternalErrorBody()};

        const auto log_level = AdjustLogLevelForCancellations(
            IsServerError(status.error_code()) ? logging::Level::kError : logging::Level::kWarning
        );
        LOG(log_level) << "Error in " << state.call_name << ": " << ex;
        auto& span = state.GetSpan();
        span.AddNonInheritableTag(tracing::kErrorFlag, true);
        span.AddNonInheritableTag(tracing::kErrorMessage, ex.what());
        span.SetLogLevel(log_level);
        return status;
    } catch (const std::exception& new_ex) {
        LOG_ERROR() << "Error in ReportCustomError: " << new_ex;
        return grpc::Status{grpc::StatusCode::INTERNAL, ""};
    }
}

grpc::Status ReportHandlerError(const std::exception& ex, CallState& state) noexcept {
    try {
        auto& span = state.GetSpan();
        const auto log_level = AdjustLogLevelForCancellations(logging::Level::kError);
        LOG(log_level) << "Uncaught exception in '" << state.call_name << "': " << ex;
        span.AddNonInheritableTag(tracing::kErrorFlag, true);
        span.AddNonInheritableTag(tracing::kErrorMessage, ex.what());
        span.SetLogLevel(log_level);
        return kUnknownErrorStatus;
    } catch (const std::exception& new_ex) {
        LOG_ERROR() << "Error in ReportHandlerError: " << new_ex;
        return grpc::Status{grpc::StatusCode::INTERNAL, ""};
    }
}

void ReportFinished(const grpc::Status& status, CallState& state) noexcept {
    try {
        state.statistics_scope.OnExplicitFinish(status.error_code());
        auto& span = state.GetSpan();
        span.AddNonInheritableTag(tracing::kGrpcCode, ugrpc::ToString(status.error_code()));
        if (!status.ok()) {
            span.AddNonInheritableTag(tracing::kErrorFlag, true);
            span.AddNonInheritableTag(tracing::kErrorMessage, status.error_message());
            const auto default_error_log_level =
                IsServerError(status.error_code()) ? logging::Level::kError : logging::Level::kWarning;
            const auto error_log_level =
                utils::FindOrDefault(state.status_codes_log_level, status.error_code(), default_error_log_level);
            span.SetLogLevel(error_log_level);
        }
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error in ReportFinished: " << ex;
    }
}

void ReportInterrupted(CallState& state) noexcept {
    try {
        state.statistics_scope.OnNetworkError();
        auto& span = state.GetSpan();
        span.AddNonInheritableTag(tracing::kErrorFlag, true);
        span.AddNonInheritableTag(tracing::kErrorMessage, "RPC interrupted");
        span.SetLogLevel(logging::Level::kWarning);
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error in ReportInterrupted: " << ex;
    }
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
