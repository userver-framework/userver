#pragma once

#include <fmt/format.h>
#include <grpcpp/impl/codegen/status.h>

namespace clients::grpc {

/// Base exception for all the client errors
class BaseError : public std::runtime_error {
 public:
  BaseError(const std::string& message);
};

/// Error occurred during rpc
class RpcError : public BaseError {
 public:
  explicit RpcError(std::string method_info, std::string additional_info = {});
  const std::string& GetRpcMethodInfo() const noexcept;

 private:
  std::string method_info_;
};

/// Attempt to write to closed stream
class StreamClosedError : public RpcError {
 public:
  StreamClosedError(std::string method_info);
};

/// Error while reading value from stream
class ValueReadError : public RpcError {
 public:
  ValueReadError(std::string method_info);
};

/// Common invocation error
class InvocationError : public RpcError {
 public:
  InvocationError(std::string method_info);
};

/// Error made from ::grpc::Status
class ErrorWithStatus : public RpcError {
 public:
  ErrorWithStatus(::grpc::StatusCode code, std::string message,
                  std::string details, std::string method_info);

  ::grpc::StatusCode GetStatusCode() const noexcept;

  const std::string& GetMessage() const noexcept;

  const std::string& GetDetails() const noexcept;

 private:
  ::grpc::StatusCode code_;
  std::string message_;
  std::string details_;
};

/// Concrete errors for all the error codes
/// For details see <grpcpp/impl/codegen/status_code_enum.h>

class CancelledError : public ErrorWithStatus {
 public:
  CancelledError(const ::grpc::Status& status, std::string method_info);
};

class UnknownError : public ErrorWithStatus {
 public:
  UnknownError(const ::grpc::Status& status, std::string method_info);
};

class InvalidArgumentError : public ErrorWithStatus {
 public:
  InvalidArgumentError(const ::grpc::Status& status, std::string method_info);
};

class DeadlineExceededError : public ErrorWithStatus {
 public:
  DeadlineExceededError(const ::grpc::Status& status, std::string method_info);
};

class NotFoundError : public ErrorWithStatus {
 public:
  NotFoundError(const ::grpc::Status& status, std::string method_info);
};

class AlreadyExistsError : public ErrorWithStatus {
 public:
  AlreadyExistsError(const ::grpc::Status& status, std::string method_info);
};

class PermissionDeniedError : public ErrorWithStatus {
 public:
  PermissionDeniedError(const ::grpc::Status& status, std::string method_info);
};

class ResourceExhaustedError : public ErrorWithStatus {
 public:
  ResourceExhaustedError(const ::grpc::Status& status, std::string method_info);
};

class FailedPreconditionError : public ErrorWithStatus {
 public:
  FailedPreconditionError(const ::grpc::Status& status,
                          std::string method_info);
};

class AbortedError : public ErrorWithStatus {
 public:
  AbortedError(const ::grpc::Status& status, std::string method_info);
};

class OutOfRangeError : public ErrorWithStatus {
 public:
  OutOfRangeError(const ::grpc::Status& status, std::string method_info);
};

class UnimplementedError : public ErrorWithStatus {
 public:
  UnimplementedError(const ::grpc::Status& status, std::string method_info);
};

class InternalError : public ErrorWithStatus {
 public:
  InternalError(const ::grpc::Status& status, std::string method_info);
};

class UnavailableError : public ErrorWithStatus {
 public:
  UnavailableError(const ::grpc::Status& status, std::string method_info);
};

class DataLossError : public ErrorWithStatus {
 public:
  DataLossError(const ::grpc::Status& status, std::string method_info);
};

class UnauthenticatedError : public ErrorWithStatus {
 public:
  UnauthenticatedError(const ::grpc::Status& status, std::string method_info);
};

/// Converts ::grpc::Status to std::exception_ptr with ErrorWithStatus.
/// status.error_code must be an actual error status code.
std::exception_ptr StatusToExceptionPtr(const ::grpc::Status& status,
                                        std::string method_info);

}  // namespace clients::grpc
