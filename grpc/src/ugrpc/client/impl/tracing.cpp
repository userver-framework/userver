#include <ugrpc/client/impl/tracing.hpp>

#include <userver/tracing/tags.hpp>

#include <userver/ugrpc/status_codes.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

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
