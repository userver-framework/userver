#pragma once

/// @file
/// @brief Exceptions thrown by gRPC client validator middleware

#include <userver/grpc-protovalidate/buf_validate.hpp>
#include <userver/ugrpc/client/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace grpc_protovalidate::client {

/// @brief RPC failed due to validator internal errors.
/// In most cases the reason this exception is thrown is
/// [protovalidate](https://github.com/bufbuild/protovalidate) CEL expression
/// errors in the *proto* file.
class ValidatorError final : public ugrpc::client::RpcError {
public:
    explicit ValidatorError(std::string_view call_name);
};

/// @brief RPC failed due to violations in the response
///        [protovalidate](https://github.com/bufbuild/protovalidate) constraints.
class ResponseError final : public ugrpc::client::RpcError {
public:
    ResponseError(std::string_view call_name, buf::validate::ValidationResult result);

    const buf::validate::ValidationResult& GetErrorInfo() const;

private:
    buf::validate::ValidationResult error_info_;
};

}  // namespace grpc_protovalidate::client

USERVER_NAMESPACE_END
