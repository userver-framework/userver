#include <userver/grpc-protovalidate/client/exceptions.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace grpc_protovalidate::client {

ValidatorError::ValidatorError(std::string_view call_name)
    : ugrpc::client::RpcError(call_name, "validator internal error") {}

ResponseError::ResponseError(std::string_view call_name, buf::validate::ValidationResult error_info)
    : ugrpc::client::RpcError(
          call_name,
          fmt::format("response violates constraints (count={})", error_info.violations_size())
      ),
      error_info_(std::move(error_info)) {}

const buf::validate::ValidationResult& ResponseError::GetErrorInfo() const { return error_info_; }

}  // namespace grpc_protovalidate::client

USERVER_NAMESPACE_END
