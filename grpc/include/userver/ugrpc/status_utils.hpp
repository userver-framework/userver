#pragma once

/// @file userver/ugrpc/status_utils.hpp
/// @brief Utilities for @c grpc::Status and @c google::rpc::Status types.

#include <optional>
#include <string>

#include <google/rpc/status.pb.h>

#include <grpcpp/support/status.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

/// @brief Converts @c google::rpc::Status from googleapis to @c grpc::Status .
/// @c google::rpc::Status is used to initialize @c grpc::Status code and
/// message and also added to status details.
[[nodiscard]] grpc::Status ToGrpcStatus(const google::rpc::Status& gstatus);

/// @brief Creates @c google::rpc::Status parsing it from  @c grpc::Status
///        details.
[[nodiscard]] std::optional<google::rpc::Status> ToGoogleRpcStatus(const grpc::Status& status);

/// @brief Outputs @a status to string using protobuf's text format.
[[nodiscard]] std::string GetGStatusLimitedMessage(const google::rpc::Status& status);

}  // namespace ugrpc

USERVER_NAMESPACE_END
