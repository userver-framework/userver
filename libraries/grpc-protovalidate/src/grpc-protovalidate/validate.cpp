#include <userver/grpc-protovalidate/validate.hpp>

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <fmt/core.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_ptr_field.h>
#include <google/rpc/status.pb.h>
#include <grpcpp/support/status.h>

#include <grpc-protovalidate/impl/utils.hpp>
#include <userver/compiler/thread_local.hpp>
#include <userver/grpc-protovalidate/buf_validate.hpp>
#include <userver/ugrpc/status_utils.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace grpc_protovalidate {

namespace {

compiler::ThreadLocal kValidatorFactory = [] { return impl::CreateProtoValidatorFactory(); };

}  // namespace

ValidationError::ValidationError(Type type, std::string description, std::string message_name)
    : type_(type),
      description_(fmt::format("Message '{}' validation error: {}", message_name, std::move(description))),
      result_(std::nullopt),
      message_name_(std::move(message_name)) {}

ValidationError::ValidationError(buf::validate::ValidationResult result, std::string message_name)
    : type_(Type::kRule),
      description_(fmt::format(
          "Message '{}' validation error: {} constraint(s) violated",
          message_name,
          result.violations_size()
      )),
      result_(std::move(result)),
      message_name_(std::move(message_name)) {}

ValidationError::Type ValidationError::GetType() const { return type_; }

const std::string& ValidationError::GetMessageName() const { return message_name_; }

const std::string& ValidationError::GetDescription() const { return description_; }

const std::vector<buf::validate::RuleViolation>& ValidationError::GetViolations() const {
    if (GetType() == Type::kInternal || !result_.has_value()) {
        static constexpr std::vector<buf::validate::RuleViolation> kNoViolations;
        return kNoViolations;
    }
    return result_->violations();
}

grpc::Status ValidationError::GetGrpcStatus(bool include_violations) const {
    google::rpc::Status gstatus;
    gstatus.set_message(GetDescription());
    switch (GetType()) {
        case Type::kInternal:
            gstatus.set_code(grpc::StatusCode::INTERNAL);
            break;
        case Type::kRule: {
            gstatus.set_code(grpc::StatusCode::INVALID_ARGUMENT);
            break;
        }
    }
    if (include_violations) {
        gstatus.add_details()->PackFrom(MakeViolationsProto());
    }
    return ugrpc::ToGrpcStatus(gstatus);
}

buf::validate::Violations ValidationError::MakeViolationsProto() const {
    buf::validate::Violations proto;
    std::transform(
        GetViolations().begin(),
        GetViolations().end(),
        RepeatedPtrFieldBackInserter(proto.mutable_violations()),
        [](const buf::validate::RuleViolation& violation) { return violation.proto(); }
    );
    return proto;
}

logging::LogHelper& operator<<(logging::LogHelper& lh, const ValidationError& error) {
    lh << error.GetDescription();
    for (const buf::validate::RuleViolation& violation : error.GetViolations()) {
        lh << violation.proto();
    }
    return lh;
}

ValidationResult::ValidationResult(ValidationError error) : error_(std::move(error)) {}

bool ValidationResult::IsSuccess() const { return !error_.has_value(); }

const ValidationError& ValidationResult::GetError() const& {
    if (IsSuccess()) {
        throw std::logic_error("Requested error for success validation result");
    }
    return error_.value();
}

ValidationError&& ValidationResult::GetError() && {
    if (IsSuccess()) {
        throw std::logic_error("Requested error for success validation result");
    }
    return std::move(error_).value();
}

ValidationResult ValidateMessage(const google::protobuf::Message& message, const ValidationParams& params) {
    auto validator_factory = kValidatorFactory.Use();
    google::protobuf::Arena arena;
    auto validator = (*validator_factory)->NewValidator(&arena, params.fail_fast);
    auto result = validator.Validate(message);
    if (!result.ok()) {
        return ValidationError(
            ValidationError::Type::kInternal,
            fmt::format(
                "internal protovalidate error (check constraints syntax in the proto file) - {}",
                result.status().ToString()
            ),
            message.GetTypeName()
        );
    }
    if (result->violations_size() != 0) {
        return ValidationError(result.value(), message.GetTypeName());
    }
    return ValidationResult();
}

}  // namespace grpc_protovalidate

USERVER_NAMESPACE_END
