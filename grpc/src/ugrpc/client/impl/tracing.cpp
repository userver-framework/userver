#include <ugrpc/client/impl/tracing.hpp>

#include <userver/tracing/opentelemetry.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/source_location.hpp>

#include <ugrpc/impl/rpc_metadata.hpp>
#include <userver/ugrpc/client/impl/call_state.hpp>
#include <userver/ugrpc/impl/to_string.hpp>
#include <userver/ugrpc/status_codes.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

namespace {

constexpr std::string_view kDefaultOtelTraceFlags = "01";

void SetErrorForSpan(tracing::Span& span, std::string_view error_message) {
    span.SetLogLevel(logging::Level::kWarning);
    span.AddTag(tracing::kErrorFlag, true);
    span.AddTag(tracing::kErrorMessage, std::string{error_message});
}

}  // namespace

void SetupSpan(
    std::optional<tracing::InPlaceSpan>& span_holder,
    grpc::ClientContext& context,
    std::string_view call_name
) {
    UASSERT(!span_holder);
    span_holder.emplace(utils::StrCat("external_grpc/", call_name), utils::impl::SourceLocation::Current());
    auto& span = span_holder->Get();

    span.DetachFromCoroStack();

    if (const auto span_id = span.GetSpanIdForChildLogs()) {
        context.AddMetadata(ugrpc::impl::kXYaTraceId, ugrpc::impl::ToGrpcString(span.GetTraceId()));
        context.AddMetadata(ugrpc::impl::kXYaSpanId, grpc::string{*span_id});
        context.AddMetadata(ugrpc::impl::kXYaRequestId, ugrpc::impl::ToGrpcString(span.GetLink()));

        auto traceparent =
            tracing::opentelemetry::BuildTraceParentHeader(span.GetTraceId(), *span_id, kDefaultOtelTraceFlags);

        if (!traceparent.has_value()) {
            LOG_LIMITED_DEBUG(
            ) << fmt::format("Cannot build opentelemetry traceparent header ({})", traceparent.error());
            return;
        }
        context.AddMetadata(ugrpc::impl::kTraceParent, ugrpc::impl::ToGrpcString(traceparent.value()));
    }
}

void SetErrorAndResetSpan(CallState& state, std::string_view error_message) {
    SetErrorForSpan(state.GetSpan(), error_message);
    state.ResetSpan();
}

void SetStatusAndResetSpan(CallState& state, const grpc::Status& status) {
    state.GetSpan().AddTag("grpc_code", ugrpc::ToString(status.error_code()));
    if (!status.ok()) {
        SetErrorForSpan(state.GetSpan(), status.error_message());
    }
    state.ResetSpan();
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
