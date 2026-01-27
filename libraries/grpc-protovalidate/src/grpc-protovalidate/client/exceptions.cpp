#include <userver/grpc-protovalidate/client/exceptions.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace grpc_protovalidate::client {

ValidatorError::ValidatorError(std::string_view call_name)
    : BaseError(call_name, "validator internal error (probably validation expression syntax is invalid)")
{}

MessageError::MessageError(
    std::string_view call_name,
    std::string_view additional_info,
    buf::validate::ValidationResult error_info
)
    : BaseError(call_name, additional_info),
      error_info_(std::move(error_info))
{}

const buf::validate::ValidationResult& MessageError::GetErrorInfo() const { return error_info_; }

ResponseError::ResponseError(std::string_view call_name, buf::validate::ValidationResult error_info)
    : MessageError(
          call_name,
          fmt::format("response violates constraints (count={})", error_info.violations_size()),
          std::move(error_info)
      )
{}

RequestError::RequestError(std::string_view call_name, buf::validate::ValidationResult error_info)
    : MessageError(
          call_name,
          fmt::format("request violates constraints (count={})", error_info.violations_size()),
          std::move(error_info)
      )
{}

}  // namespace grpc_protovalidate::client

USERVER_NAMESPACE_END
