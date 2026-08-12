#include <userver/ugrpc/server/impl/call_state.hpp>

#include <userver/dynamic_config/source.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/manager.hpp>
#include <userver/tracing/opentelemetry.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/from_string.hpp>
#include <userver/utils/impl/source_location.hpp>

#include <userver/ugrpc/impl/to_string.hpp>

#include <ugrpc/impl/rpc_metadata.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

namespace {

void ConstructSpan(
    std::optional<tracing::InPlaceSpan>& span_storage,
    std::string_view call_name,
    const grpc::ServerContext& server_context,
    bool otel_sampling_enabled
) {
    const auto& client_metadata = server_context.client_metadata();

    if (const auto* const traceparent = utils::FindOrNullptr(client_metadata, ugrpc::impl::kTraceParent)) {
        auto extract_result = tracing::opentelemetry::ExtractTraceParentDataView(ugrpc::impl::ToStringView(*traceparent)
        );
        if (extract_result.has_value()) {
            const auto& traceparent_data = extract_result.value();
            span_storage.emplace(
                std::string{call_name},
                traceparent_data.trace_id,
                traceparent_data.span_id,
                utils::impl::SourceLocation::Current()
            );

            const auto* const ts = utils::FindOrNullptr(client_metadata, ugrpc::impl::kTraceState);
            tracing::SetInheritedOtelTracingData(
                ts ? ugrpc::impl::ToStringView(*ts) : std::string_view{},
                traceparent_data.trace_flags
            );

            if (otel_sampling_enabled) {
                span_storage->Get().SetSampled(
                    (tracing::GetInheritedOtelTraceFlags() & tracing::OtelTraceFlags::kSampled) !=
                    tracing::OtelTraceFlags::kNoTracing
                );
            }
        } else {
            LOG_LIMITED_WARNING() << fmt::format(
                "Invalid traceparent header format ({}). Skipping Opentelemetry "
                "headers",
                extract_result.error()
            );
            span_storage.emplace(std::string{call_name}, utils::impl::SourceLocation::Current());
        }
    } else if (const auto* const trace_id = utils::FindOrNullptr(client_metadata, ugrpc::impl::kXYaTraceId)) {
        span_storage.emplace(
            std::string{call_name},
            ugrpc::impl::ToStringView(*trace_id),
            ugrpc::impl::ToStringView(utils::FindOrDefault(client_metadata, ugrpc::impl::kXYaSpanId)),
            utils::impl::SourceLocation::Current()
        );
    } else {
        span_storage.emplace(std::string{call_name}, utils::impl::SourceLocation::Current());
    }

    if (const auto* const request_id = utils::FindOrNullptr(client_metadata, ugrpc::impl::kXYaRequestId)) {
        span_storage->Get().SetParentLink(ugrpc::impl::ToStringView(*request_id));
    }
}

void AddServiceMethodTags(tracing::Span& span, std::string_view service_name, std::string_view method_name) {
    span.AddNonInheritableTag(tracing::kRpcSystem, "grpc");
    span.AddNonInheritableTag(tracing::kSpanKind, tracing::kSpanKindServer);
    span.AddNonInheritableTag(tracing::kRpcService, std::string{service_name});
    span.AddNonInheritableTag(tracing::kRpcMethod, std::string{method_name});
}

}  // namespace

CallState::CallState(CallParams&& params)
    : CallParams(std::move(params)),
      statistics_scope(method_statistics),
      config_snapshot(config_source.GetSnapshot())
{
    ConstructSpan(span, call_name, server_context, otel_trace_sampling_enabled);
    AddServiceMethodTags(span->Get(), service_name, method_name);
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
