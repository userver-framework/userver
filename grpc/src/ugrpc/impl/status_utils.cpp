#include <userver/ugrpc/impl/status_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

void ClampStatusCodeToValidRange(grpc::Status& status) {
    if (status.error_code() < 0 || status.error_code() > 16) {
        status = grpc::Status{grpc::StatusCode::UNKNOWN, status.error_message(), status.error_details()};
    }
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
