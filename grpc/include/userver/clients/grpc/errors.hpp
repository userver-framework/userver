#pragma once

#include <exception>
#include <string>
#include <string_view>

#include <grpcpp/impl/codegen/status.h>

namespace clients::grpc {

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

/// @brief RPC failed without a status
class UnknownRpcError final : public RpcError {
 public:
  UnknownRpcError(std::string_view call_name, std::string_view stage);
};

/// @brief Unexpected end-of-input in a bidirectional stream
///
/// The server has indicated end-of-input, but we haven't indicated
/// end-of-output yet, so we probably expected more data from the server.
class UnexpectedEndOfInput final : public RpcError {
 public:
  explicit UnexpectedEndOfInput(std::string_view call_name);
};

/// @brief Error with ::grpc::Status details
/// @see <grpcpp/impl/codegen/status_code_enum.h> for error code details
class ErrorWithStatus : public RpcError {
 public:
  ErrorWithStatus(std::string_view call_name, ::grpc::Status&& status);

  const ::grpc::Status& GetStatus() const noexcept;

 private:
  ::grpc::Status status_;
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
[[noreturn]] void ThrowErrorWithStatus(std::string_view call_name,
                                       ::grpc::Status&& status);
}  // namespace impl

}  // namespace clients::grpc
