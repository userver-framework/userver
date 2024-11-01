#include <userver/ugrpc/server/impl/service_worker_impl.hpp>

#include <chrono>

#include <grpc/support/time.h>

#include <userver/logging/log.hpp>
#include <userver/tracing/opentelemetry.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/from_string.hpp>
#include <userver/utils/impl/source_location.hpp>
#include <userver/utils/impl/userver_experiments.hpp>

#include <ugrpc/impl/rpc_metadata.hpp>
#include <ugrpc/server/impl/server_configs.hpp>
#include <userver/ugrpc/impl/to_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

void ReportHandlerError(
    const std::exception& ex,
    std::string_view call_name,
    tracing::Span& span,
    ugrpc::impl::RpcStatisticsScope& statistics_scope
) noexcept {
    if (engine::current_task::ShouldCancel()) {
        LOG_WARNING() << "Handler task cancelled, error in '" << call_name << "': " << ex;
        statistics_scope.OnCancelled();
        span.AddTag(tracing::kErrorMessage, "Handler task cancelled");
    } else {
        LOG_ERROR() << "Uncaught exception in '" << call_name << "': " << ex;
        span.AddTag(tracing::kErrorMessage, ex.what());
    }
    span.AddTag(tracing::kErrorFlag, true);
}

void ReportNetworkError(
    const RpcInterruptedError& ex,
    std::string_view call_name,
    tracing::Span& span,
    ugrpc::impl::RpcStatisticsScope& statistics_scope
) noexcept {
    if (engine::current_task::ShouldCancel()) {
        LOG_WARNING() << "Handler task cancelled, error in '" << call_name << "': " << ex;
        statistics_scope.OnCancelled();
        span.AddTag(tracing::kErrorMessage, "Handler task cancelled");
    } else {
        LOG_WARNING() << "Network error in '" << call_name << "': " << ex;
        statistics_scope.OnNetworkError();
        span.AddTag(tracing::kErrorMessage, ex.what());
    }
    span.AddTag(tracing::kErrorFlag, true);
}

void ReportCustomError(
    const USERVER_NAMESPACE::server::handlers::CustomHandlerException& ex,
    CallAnyBase& call,
    tracing::Span& span
) {
    if (!call.IsFinished()) {
        call.FinishWithError({CustomStatusToGrpc(ex.GetCode()), ugrpc::impl::ToGrpcString(ex.GetExternalErrorBody())});
    }

    LOG_WARNING() << "Error in " << call.GetCallName() << ": " << ex;
    span.AddTag(tracing::kErrorFlag, true);
    span.AddTag(tracing::kErrorMessage, ex.what());
}

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

    context.AddInitialMetadata(ugrpc::impl::kXYaTraceId, ugrpc::impl::ToGrpcString(span.GetTraceId()));
    context.AddInitialMetadata(ugrpc::impl::kXYaSpanId, ugrpc::impl::ToGrpcString(span.GetSpanId()));
    context.AddInitialMetadata(ugrpc::impl::kXYaRequestId, ugrpc::impl::ToGrpcString(span.GetLink()));
}

void ParseGenericCallName(
    std::string_view generic_call_name,
    std::string_view& call_name,
    std::string_view& service_name,
    std::string_view& method_name
) {
    UINVARIANT(
        !generic_call_name.empty() && generic_call_name[0] == '/', "Generic service call name must start with a '/'"
    );
    generic_call_name.remove_prefix(1);
    const auto slash_pos = generic_call_name.find('/');
    UINVARIANT(slash_pos != std::string_view::npos, "Generic service call name must contain a '/'");
    call_name = generic_call_name;
    service_name = generic_call_name.substr(0, slash_pos);
    method_name = generic_call_name.substr(slash_pos + 1);
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
