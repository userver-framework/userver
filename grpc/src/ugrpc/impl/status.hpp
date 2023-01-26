#pragma once

#include <optional>
#include <string>

#include <google/rpc/status.pb.h>
#include <grpcpp/support/status.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

[[nodiscard]] grpc::Status ToGrpcStatus(const google::rpc::Status& gstatus);

[[nodiscard]] std::optional<google::rpc::Status> ToGoogleRpcStatus(
    const grpc::Status& status);

[[nodiscard]] std::string GetGStatusLimitedMessage(
    const google::rpc::Status& status);

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
