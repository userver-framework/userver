#include <userver/clients/grpc/errors.hpp>

namespace clients::grpc {

BaseError::BaseError(const std::string& message)
    : std::runtime_error(message) {}

RpcError::RpcError(std::string method_info, std::string additional_info)
    : BaseError(fmt::format("{}: {}", method_info, std::move(additional_info))),
      method_info_(std::move(method_info)) {}

const std::string& RpcError::GetRpcMethodInfo() const noexcept {
  return method_info_;
}

StreamClosedError::StreamClosedError(std::string method_info)
    : RpcError(std::move(method_info), "Stream is closed") {}

ValueReadError::ValueReadError(std::string method_info)
    : RpcError(std::move(method_info), "Value read failure") {}

InvocationError::InvocationError(std::string method_info)
    : RpcError(std::move(method_info), "Invocation failed") {}

ErrorWithStatus::ErrorWithStatus(::grpc::StatusCode code, std::string message,
                                 std::string details, std::string method_info)
    : RpcError(std::move(method_info),
               fmt::format("code: {}; message: {}", code, message)),
      code_(code),
      message_(std::move(message)),
      details_(std::move(details)) {}

::grpc::StatusCode ErrorWithStatus::GetStatusCode() const noexcept {
  return code_;
}

const std::string& ErrorWithStatus::GetMessage() const noexcept {
  return message_;
}

const std::string& ErrorWithStatus::GetDetails() const noexcept {
  return details_;
}

CancelledError::CancelledError(const ::grpc::Status& status,
                               std::string method_info)
    : ErrorWithStatus(::grpc::StatusCode::CANCELLED, status.error_message(),
                      status.error_details(), std::move(method_info)) {}

UnknownError::UnknownError(const ::grpc::Status& status,
                           std::string method_info)
    : ErrorWithStatus(::grpc::StatusCode::UNKNOWN, status.error_message(),
                      status.error_details(), std::move(method_info)) {}

InvalidArgumentError::InvalidArgumentError(const ::grpc::Status& status,
                                           std::string method_info)
    : ErrorWithStatus(::grpc::StatusCode::INVALID_ARGUMENT,
                      status.error_message(), status.error_details(),
                      std::move(method_info)) {}

DeadlineExceededError::DeadlineExceededError(const ::grpc::Status& status,
                                             std::string method_info)
    : ErrorWithStatus(::grpc::StatusCode::DEADLINE_EXCEEDED,
                      status.error_message(), status.error_details(),
                      std::move(method_info)) {}

NotFoundError::NotFoundError(const ::grpc::Status& status,
                             std::string method_info)
    : ErrorWithStatus(::grpc::StatusCode::NOT_FOUND, status.error_message(),
                      status.error_details(), std::move(method_info)) {}

AlreadyExistsError::AlreadyExistsError(const ::grpc::Status& status,
                                       std::string method_info)
    : ErrorWithStatus(::grpc::StatusCode::ALREADY_EXISTS,
                      status.error_message(), status.error_details(),
                      std::move(method_info)) {}

PermissionDeniedError::PermissionDeniedError(const ::grpc::Status& status,
                                             std::string method_info)
    : ErrorWithStatus(::grpc::StatusCode::PERMISSION_DENIED,
                      status.error_message(), status.error_details(),
                      std::move(method_info)) {}

ResourceExhaustedError::ResourceExhaustedError(const ::grpc::Status& status,
                                               std::string method_info)
    : ErrorWithStatus(::grpc::StatusCode::RESOURCE_EXHAUSTED,
                      status.error_message(), status.error_details(),
                      std::move(method_info)) {}

FailedPreconditionError::FailedPreconditionError(const ::grpc::Status& status,
                                                 std::string method_info)
    : ErrorWithStatus(::grpc::StatusCode::FAILED_PRECONDITION,
                      status.error_message(), status.error_details(),
                      std::move(method_info)) {}

AbortedError::AbortedError(const ::grpc::Status& status,
                           std::string method_info)
    : ErrorWithStatus(::grpc::StatusCode::ABORTED, status.error_message(),
                      status.error_details(), std::move(method_info)) {}

OutOfRangeError::OutOfRangeError(const ::grpc::Status& status,
                                 std::string method_info)
    : ErrorWithStatus(::grpc::StatusCode::OUT_OF_RANGE, status.error_message(),
                      status.error_details(), std::move(method_info)) {}

UnimplementedError::UnimplementedError(const ::grpc::Status& status,
                                       std::string method_info)
    : ErrorWithStatus(::grpc::StatusCode::UNIMPLEMENTED, status.error_message(),
                      status.error_details(), std::move(method_info)) {}

InternalError::InternalError(const ::grpc::Status& status,
                             std::string method_info)
    : ErrorWithStatus(::grpc::StatusCode::INTERNAL, status.error_message(),
                      status.error_details(), std::move(method_info)) {}

UnavailableError::UnavailableError(const ::grpc::Status& status,
                                   std::string method_info)
    : ErrorWithStatus(::grpc::StatusCode::UNAVAILABLE, status.error_message(),
                      status.error_details(), std::move(method_info)) {}

DataLossError::DataLossError(const ::grpc::Status& status,
                             std::string method_info)
    : ErrorWithStatus(::grpc::StatusCode::DATA_LOSS, status.error_message(),
                      status.error_details(), std::move(method_info)) {}

UnauthenticatedError::UnauthenticatedError(const ::grpc::Status& status,
                                           std::string method_info)
    : ErrorWithStatus(::grpc::StatusCode::UNAUTHENTICATED,
                      status.error_message(), status.error_details(),
                      std::move(method_info)) {}

std::exception_ptr StatusToExceptionPtr(const ::grpc::Status& status,
                                        std::string method_info) {
  switch (status.error_code()) {
    case ::grpc::StatusCode::CANCELLED:
      return std::make_exception_ptr(
          CancelledError(status, std::move(method_info)));
    case ::grpc::StatusCode::UNKNOWN:
      return std::make_exception_ptr(
          UnknownError(status, std::move(method_info)));
    case ::grpc::StatusCode::INVALID_ARGUMENT:
      return std::make_exception_ptr(
          InvalidArgumentError(status, std::move(method_info)));
    case ::grpc::StatusCode::DEADLINE_EXCEEDED:
      return std::make_exception_ptr(
          DeadlineExceededError(status, std::move(method_info)));
    case ::grpc::StatusCode::NOT_FOUND:
      return std::make_exception_ptr(
          NotFoundError(status, std::move(method_info)));
    case ::grpc::StatusCode::ALREADY_EXISTS:
      return std::make_exception_ptr(
          AlreadyExistsError(status, std::move(method_info)));
    case ::grpc::StatusCode::PERMISSION_DENIED:
      return std::make_exception_ptr(
          PermissionDeniedError(status, std::move(method_info)));
    case ::grpc::StatusCode::RESOURCE_EXHAUSTED:
      return std::make_exception_ptr(
          ResourceExhaustedError(status, std::move(method_info)));
    case ::grpc::StatusCode::FAILED_PRECONDITION:
      return std::make_exception_ptr(
          FailedPreconditionError(status, std::move(method_info)));
    case ::grpc::StatusCode::ABORTED:
      return std::make_exception_ptr(
          AbortedError(status, std::move(method_info)));
    case ::grpc::StatusCode::OUT_OF_RANGE:
      return std::make_exception_ptr(
          OutOfRangeError(status, std::move(method_info)));
    case ::grpc::StatusCode::UNIMPLEMENTED:
      return std::make_exception_ptr(
          UnimplementedError(status, std::move(method_info)));
    case ::grpc::StatusCode::INTERNAL:
      return std::make_exception_ptr(
          InternalError(status, std::move(method_info)));
    case ::grpc::StatusCode::UNAVAILABLE:
      return std::make_exception_ptr(
          UnavailableError(status, std::move(method_info)));
    case ::grpc::StatusCode::DATA_LOSS:
      return std::make_exception_ptr(
          DataLossError(status, std::move(method_info)));
    case ::grpc::StatusCode::UNAUTHENTICATED:
      return std::make_exception_ptr(
          UnauthenticatedError(status, std::move(method_info)));
    default:
      throw std::logic_error("Unexpected status code");
  }
}
}  // namespace clients::grpc
