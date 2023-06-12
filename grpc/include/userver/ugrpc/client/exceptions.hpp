#pragma once

/// @file userver/ugrpc/client/exceptions.hpp
/// @brief Exceptions thrown by gRPC client streams

#include <exception>
#include <optional>
#include <string>
#include <string_view>

#include <google/rpc/status.pb.h>
#include <grpcpp/support/status.h>

USERVER_NAMESPACE_BEGIN

/// Asynchronous gRPC driver
namespace ugrpc {}

/// Client-side utilities
namespace ugrpc::client {

/// @brief Base exception for all the client errors
class BaseError : public std::exception {
 public:
  explicit BaseError(std::string message);

  const char* what() const noexcept override;

 private:
  std::string message_;
};

/// @brief Error during an RPC
class RpcError : public BaseError {
 public:
  RpcError(std::string_view call_name, std::string_view additional_info);
};

/// @brief RPC failed without a status. This means that either the call got
/// cancelled using `TryCancel`, the deadline has expired, or the channel is
/// broken.
class RpcInterruptedError final : public RpcError {
 public:
  RpcInterruptedError(std::string_view call_name, std::string_view stage);
};

/// @brief Error with grpc::Status details
/// @see <grpcpp/impl/codegen/status_code_enum.h> for error code details
class ErrorWithStatus : public RpcError {
 public:
  ErrorWithStatus(std::string_view call_name, grpc::Status&& status,
                  std::optional<google::rpc::Status>&& gstatus,
                  std::optional<std::string>&& message);

  const grpc::Status& GetStatus() const noexcept;

  const std::optional<google::rpc::Status>& GetGStatus() const noexcept;

  const std::optional<std::string>& GetGStatusString() const noexcept;

 private:
  grpc::Status status_;
  std::optional<google::rpc::Status> gstatus_;
  std::optional<std::string> gstatus_string_;
};

/// @brief Concrete errors for all the error codes
/// @see <grpcpp/impl/codegen/status_code_enum.h> for error code details
/// @{
class CancelledError final : public ErrorWithStatus {
 public:
  using ErrorWithStatus::ErrorWithStatus;
};

class UnknownError final : public ErrorWithStatus {
 public:
  using ErrorWithStatus::ErrorWithStatus;
};

class InvalidArgumentError final : public ErrorWithStatus {
 public:
  using ErrorWithStatus::ErrorWithStatus;
};

class DeadlineExceededError final : public ErrorWithStatus {
 public:
  using ErrorWithStatus::ErrorWithStatus;
};

class NotFoundError final : public ErrorWithStatus {
 public:
  using ErrorWithStatus::ErrorWithStatus;
};

class AlreadyExistsError final : public ErrorWithStatus {
 public:
  using ErrorWithStatus::ErrorWithStatus;
};

class PermissionDeniedError final : public ErrorWithStatus {
 public:
  using ErrorWithStatus::ErrorWithStatus;
};

class ResourceExhaustedError final : public ErrorWithStatus {
 public:
  using ErrorWithStatus::ErrorWithStatus;
};

class FailedPreconditionError final : public ErrorWithStatus {
 public:
  using ErrorWithStatus::ErrorWithStatus;
};

class AbortedError final : public ErrorWithStatus {
 public:
  using ErrorWithStatus::ErrorWithStatus;
};

class OutOfRangeError final : public ErrorWithStatus {
 public:
  using ErrorWithStatus::ErrorWithStatus;
};

class UnimplementedError final : public ErrorWithStatus {
 public:
  using ErrorWithStatus::ErrorWithStatus;
};

class InternalError final : public ErrorWithStatus {
 public:
  using ErrorWithStatus::ErrorWithStatus;
};

class UnavailableError final : public ErrorWithStatus {
 public:
  using ErrorWithStatus::ErrorWithStatus;
};

class DataLossError final : public ErrorWithStatus {
 public:
  using ErrorWithStatus::ErrorWithStatus;
};

class UnauthenticatedError final : public ErrorWithStatus {
 public:
  using ErrorWithStatus::ErrorWithStatus;
};
/// @}

namespace impl {

[[noreturn]] void ThrowErrorWithStatus(
    std::string_view call_name, grpc::Status&& status,
    std::optional<google::rpc::Status>&& gstatus,
    std::optional<std::string>&& gstatus_string);

}  // namespace impl

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
