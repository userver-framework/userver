#include <userver/ugrpc/server/impl/call_processor.hpp>

#include <chrono>

#include <userver/logging/log.hpp>
#include <userver/tracing/opentelemetry.hpp>
#include <userver/tracing/tags.hpp>

#include <ugrpc/impl/rpc_metadata.hpp>
#include <userver/ugrpc/impl/to_string.hpp>
#include <userver/ugrpc/server/impl/error_code.hpp>
#include <userver/ugrpc/status_codes.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

grpc::Status ReportHandlerError(
    const std::exception& ex,
    std::string_view call_name,
    tracing::Span& span,
    ugrpc::impl::RpcStatisticsScope& statistics_scope
) noexcept {
    if (engine::current_task::ShouldCancel()) {
        LOG_WARNING() << "Handler task cancelled, error in '" << call_name << "': " << ex;
        statistics_scope.OnCancelled();
        span.AddTag(tracing::kErrorMessage, "Handler task cancelled");
        span.SetLogLevel(logging::Level::kWarning);
    } else {
        LOG_ERROR() << "Uncaught exception in '" << call_name << "': " << ex;
        span.AddTag(tracing::kErrorMessage, ex.what());
        span.SetLogLevel(logging::Level::kError);
    }
    span.AddTag(tracing::kErrorFlag, true);
    if (engine::current_task::ShouldCancel()) {
        return grpc::Status{grpc::StatusCode::CANCELLED, ""};
    }
    return kUnknownErrorStatus;
}

grpc::Status ReportNetworkError(
    const RpcInterruptedError& ex,
    std::string_view call_name,
    tracing::Span& span,
    ugrpc::impl::RpcStatisticsScope& statistics_scope
) noexcept {
    if (engine::current_task::ShouldCancel()) {
        LOG_WARNING() << "Handler task cancelled, error in '" << call_name << "': " << ex;
        statistics_scope.OnCancelled();
        span.AddTag(tracing::kErrorMessage, "Handler task cancelled");
        span.SetLogLevel(logging::Level::kWarning);
    } else {
        LOG_WARNING() << "Network error in '" << call_name << "': " << ex;
        statistics_scope.OnNetworkError();
        span.AddTag(tracing::kErrorMessage, ex.what());
        span.SetLogLevel(logging::Level::kWarning);
    }
    span.AddTag(tracing::kErrorFlag, true);
    return grpc::Status{grpc::StatusCode::CANCELLED, ""};
}

grpc::Status ReportCustomError(
    const USERVER_NAMESPACE::server::handlers::CustomHandlerException& ex,
    std::string_view call_name,
    tracing::Span& span
) {
    grpc::Status status{CustomStatusToGrpc(ex.GetCode()), ugrpc::impl::ToGrpcString(ex.GetExternalErrorBody())};

    const auto log_level = IsServerError(status.error_code()) ? logging::Level::kError : logging::Level::kWarning;
    LOG(log_level) << "Error in " << call_name << ": " << ex;
    span.AddTag(tracing::kErrorFlag, true);
    span.AddTag(tracing::kErrorMessage, ex.what());
    span.SetLogLevel(log_level);
    return status;
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
