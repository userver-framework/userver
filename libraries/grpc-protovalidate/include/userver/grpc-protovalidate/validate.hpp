#pragma once

/// @file
/// @brief Coroutine-safe wrappers around protovalidate-cc.

#include <optional>
#include <string>
#include <vector>

#include <google/protobuf/message.h>
#include <grpcpp/support/status.h>

#include <userver/grpc-protovalidate/buf_validate.hpp>
#include <userver/logging/log_helper_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace grpc_protovalidate {

class ValidationError {
public:
    /// @brief Type of validation error.
    enum class Type {

        /// @brief Validation failed due to validator internal errors.
        /// In most cases this indicates [protovalidate](https://github.com/bufbuild/protovalidate)
        /// CEL expression errors in the *proto* file.
        kInternal = 1,

        /// @brief Validation failed due to violations of
        /// [protovalidate](https://github.com/bufbuild/protovalidate) constraints by the input message.
        kRule = 2
    };

    ValidationError(Type type, std::string description, std::string message_name);

    /// @brief Creates an error of type @c Type::kRule.
    ValidationError(buf::validate::ValidationResult result, std::string message_name);

    /// @brief Get error type.
    Type GetType() const;

    /// @brief A fully-qualified name of the message type this error originates from.
    const std::string& GetMessageName() const;

    /// @brief A human-readable description of the error.
    const std::string& GetDescription() const;

    /// @brief A list of found contraint violations.
    ///
    /// The list is empty if this is a @c Type::kInternal error.
    const std::vector<buf::validate::RuleViolation>& GetViolations() const;

    /// @brief Constructs a @c grpc::Status of this error.
    ///
    /// Message contains a short human-readable representation of the error.
    /// If include_violations is true, details contain the list of violations. Otherwise, details are empty.
    grpc::Status GetGrpcStatus(bool include_violations = true) const;

private:
    buf::validate::Violations MakeViolationsProto() const;

    Type type_;
    std::string description_;
    std::optional<buf::validate::ValidationResult> result_;
    std::string message_name_;
};

logging::LogHelper& operator<<(logging::LogHelper& lh, const ValidationError& error);

class ValidationResult {
public:
    ValidationResult() = default;
    ValidationResult(ValidationError error);

    /// @returns `true` iff the validation found no violations.
    bool IsSuccess() const;

    explicit operator bool() const { return IsSuccess(); }

    /// @returns ValidationError with the description of the violations.
    /// @pre `IsSuccess() == true`.
    const ValidationError& GetError() const&;

    /// @returns ValidationError with the description of the violations.
    /// @pre `IsSuccess() == true`.
    ValidationError&& GetError() &&;

private:
    std::optional<ValidationError> error_;
};

struct ValidationParams {
    /// @brief If true, does not check remaining constraints after the first error is encountered.
    bool fail_fast = false;
};

/// @brief Coroutine-safe wrapper around Validate from protovalidate-cc.
///
/// @returns std::nullopt if no violations have been found.
/// Using @c buf::validate::ValidatorFactory is not safe in a coroutine context and may cause crashes.
/// This method uses ThreadLocal to ensure no unexpected coroutine-context switches occur during validation.
ValidationResult ValidateMessage(const google::protobuf::Message& message, const ValidationParams& params = {});

}  // namespace grpc_protovalidate

USERVER_NAMESPACE_END
