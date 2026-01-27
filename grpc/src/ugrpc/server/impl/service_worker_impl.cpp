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
#include <userver/ugrpc/impl/to_string.hpp>
#include <userver/ugrpc/status_codes.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

GenericMethodParseResults ParseGenericMethodName(std::string_view generic_method_name) {
    UINVARIANT(
        !generic_method_name.empty() && generic_method_name[0] == '/',
        "Generic service method name must start with a '/'"
    );
    const auto slash_pos = generic_method_name.find('/', 1);
    UINVARIANT(slash_pos != std::string_view::npos, "Generic service method name must contain a '/'");

    /*
     expected generic_method_name format:

     "/<service-name>/<method-name>"

    */
    auto call_name = generic_method_name.substr(1);
    auto service_name = generic_method_name.substr(1, slash_pos - 1);
    auto method_name = generic_method_name.substr(slash_pos + 1);

    return GenericMethodParseResults{call_name, service_name, method_name};
}

void ConstructSpan(
    std::optional<tracing::InPlaceSpan>& span_storage,
    std::string_view call_name,
    const grpc::ServerContext& server_context
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

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
