#include <ugrpc/impl/span.hpp>

#include <userver/tracing/tags.hpp>
#include <userver/ugrpc/status_codes.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

void UpdateSpanWithStatus(tracing::Span& span, const grpc::Status& status) {
    span.AddTag("grpc_code", ugrpc::ToString(status.error_code()));
    if (!status.ok()) {
        span.AddTag(tracing::kErrorFlag, true);
        span.AddTag(tracing::kErrorMessage, status.error_message());
        span.SetLogLevel(logging::Level::kWarning);
    }
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
