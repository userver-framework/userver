#pragma once

/// @file userver/ugrpc/client/exceptions.hpp
/// @brief Exceptions thrown by gRPC client streams

#include <exception>
#include <string>
#include <string_view>

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

/// @brief RPC failed due to task cancellation
class RpcCancelledError final : public RpcError {
public:
    RpcCancelledError(std::string_view call_name, std::string_view stage);
};

/// @brief Error with grpc::Status details
/// @see <grpcpp/impl/codegen/status_code_enum.h> for error code details
class ErrorWithStatus : public RpcError {
public:
    ErrorWithStatus(std::string_view call_name, grpc::Status&& status);

    const grpc::Status& GetStatus() const noexcept;

private:
    grpc::Status status_;
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

[[noreturn]] void ThrowErrorWithStatus(std::string_view call_name, grpc::Status&& status);

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
