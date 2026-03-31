#pragma once

/// @file
/// @brief Exceptions thrown by gRPC client validator middleware

#include <userver/grpc-protovalidate/buf_validate.hpp>
#include <userver/ugrpc/client/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace grpc_protovalidate::client {

/// @brief Base grpc_protovalidate error.
class BaseError : public ugrpc::client::RpcError {
public:
    using ugrpc::client::RpcError::RpcError;
};

/// @brief Base grpc_protovalidate error with @ref buf::validate::ValidationResult.
class MessageError : public BaseError {
public:
    MessageError(std::string_view call_name, std::string_view additional_info, buf::validate::ValidationResult result);

    const buf::validate::ValidationResult& GetErrorInfo() const;

private:
    buf::validate::ValidationResult error_info_;
};

/// @brief RPC failed due to validator internal errors.
/// In most cases the reason this exception is thrown is
/// [protovalidate](https://github.com/bufbuild/protovalidate) CEL expression
/// errors in the *proto* file.
class ValidatorError final : public BaseError {
public:
    explicit ValidatorError(std::string_view call_name);
};

/// @brief RPC failed due to violations in the response
///        [protovalidate](https://github.com/bufbuild/protovalidate) constraints.
class ResponseError final : public MessageError {
public:
    ResponseError(std::string_view call_name, buf::validate::ValidationResult result);
};

/// @brief RPC failed due to violations in the request
///        [protovalidate](https://github.com/bufbuild/protovalidate) constraints.
class RequestError final : public MessageError {
public:
    RequestError(std::string_view call_name, buf::validate::ValidationResult result);
};

}  // namespace grpc_protovalidate::client

USERVER_NAMESPACE_END
