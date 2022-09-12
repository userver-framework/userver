#include <userver/ugrpc/impl/span.hpp>

#include <string_view>

#include <userver/tracing/tags.hpp>
#include <userver/ugrpc/impl/status_codes.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

void UpdateSpanWithStatus(tracing::Span& span, const grpc::Status& status) {
  static const std::string kGrpcStatusTag{"grpc_status"};
  span.AddTag(kGrpcStatusTag, std::string{ToString(status.error_code())});
  if (!status.ok()) {
    span.AddTag(tracing::kErrorFlag, true);
    span.AddTag(tracing::kErrorMessage, status.error_message());
    span.SetLogLevel(logging::Level::kWarning);
  }
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
