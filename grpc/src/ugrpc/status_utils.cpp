#include <userver/ugrpc/status_utils.hpp>

#include <ugrpc/impl/protobuf_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

grpc::Status ToGrpcStatus(const google::rpc::Status& gstatus) {
    grpc::StatusCode code = grpc::StatusCode::UNKNOWN;
    if (gstatus.code() >= grpc::StatusCode::OK && gstatus.code() <= grpc::StatusCode::UNAUTHENTICATED) {
        code = static_cast<grpc::StatusCode>(gstatus.code());
    }
    return grpc::Status(code, gstatus.message(), gstatus.SerializeAsString());
}

std::optional<google::rpc::Status> ToGoogleRpcStatus(const grpc::Status& status) {
    if (status.error_details().empty()) {
        return {};
    }
    google::rpc::Status gstatus;
    if (!gstatus.ParseFromString(status.error_details())) {
        return {};
    }
    return gstatus;
}

std::string GetGStatusLimitedMessage(const google::rpc::Status& status) {
    constexpr std::size_t kLengthLimit = 1024;
    return impl::ToLimitedDebugString(status, kLengthLimit);
}

}  // namespace ugrpc

USERVER_NAMESPACE_END
