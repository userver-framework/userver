#include <userver/ugrpc/client/impl/retry_policy.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

/// [retryable]
bool IsRetryable(grpc::StatusCode status_code) noexcept {
    switch (status_code) {
        case grpc::StatusCode::CANCELLED:
        case grpc::StatusCode::UNKNOWN:
        case grpc::StatusCode::DEADLINE_EXCEEDED:
        case grpc::StatusCode::ABORTED:
        case grpc::StatusCode::INTERNAL:
        case grpc::StatusCode::UNAVAILABLE:
            return true;

        default:
            return false;
    }
}
/// [retryable]

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
