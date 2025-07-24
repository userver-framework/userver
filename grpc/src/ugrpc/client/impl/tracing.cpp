#include <ugrpc/client/impl/tracing.hpp>

#include <userver/tracing/opentelemetry.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/source_location.hpp>

#include <ugrpc/impl/rpc_metadata.hpp>
#include <userver/ugrpc/impl/to_string.hpp>
#include <userver/ugrpc/status_codes.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

namespace {

constexpr std::string_view kDefaultOtelTraceFlags = "01";

}  // namespace

void SetupSpan(std::optional<tracing::InPlaceSpan>& span_holder, std::string_view call_name) {
    UASSERT(!span_holder);
    span_holder.emplace(utils::StrCat("external_grpc/", call_name), utils::impl::SourceLocation::Current());
    auto& span = span_holder->Get();
    span.DetachFromCoroStack();
}

void AddTracingMetadata(grpc::ClientContext& client_context, tracing::Span& span) {
    if (const auto span_id = span.GetSpanIdForChildLogs()) {
        client_context.AddMetadata(ugrpc::impl::kXYaTraceId, ugrpc::impl::ToGrpcString(span.GetTraceId()));
        client_context.AddMetadata(ugrpc::impl::kXYaSpanId, ugrpc::impl::ToGrpcString(*span_id));
        client_context.AddMetadata(ugrpc::impl::kXYaRequestId, ugrpc::impl::ToGrpcString(span.GetLink()));

        auto traceparent =
            tracing::opentelemetry::BuildTraceParentHeader(span.GetTraceId(), *span_id, kDefaultOtelTraceFlags);

        if (!traceparent.has_value()) {
            LOG_LIMITED_DEBUG(
            ) << fmt::format("Cannot build opentelemetry traceparent header ({})", traceparent.error());
            return;
        }
        client_context.AddMetadata(ugrpc::impl::kTraceParent, ugrpc::impl::ToGrpcString(traceparent.value()));
    }
}

void SetErrorForSpan(tracing::Span& span, std::string_view error_message) noexcept {
    try {
        span.SetLogLevel(logging::Level::kWarning);
        span.AddTag(tracing::kErrorFlag, true);
        span.AddTag(tracing::kErrorMessage, std::string{error_message});
    } catch (const std::exception& ex) {
        LOG_LIMITED_ERROR() << "Can not set error for span: " << ex;
    }
}

void SetStatusForSpan(tracing::Span& span, const grpc::Status& status) noexcept {
    try {
        span.AddTag("grpc_code", ugrpc::ToString(status.error_code()));
        if (!status.ok()) {
            SetErrorForSpan(span, status.error_message());
        }
    } catch (const std::exception& ex) {
        LOG_LIMITED_ERROR() << "Can not set status for span: " << ex;
    }
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
