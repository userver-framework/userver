#pragma once

/// @file userver/ugrpc/rich_status.hpp
/// @brief userver wrapper for `google::rpc::Status`

#include <optional>

#include <google/protobuf/any.pb.h>
#include <google/protobuf/message.h>
#include <google/rpc/error_details.pb.h>
#include <google/rpc/status.pb.h>
#include <grpcpp/support/status.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

/// @brief A wrapper around `google::rpc::Status` that provides a convenient API
/// for creating and managing gRPC status responses with rich error details.
///
/// `RichStatus` allows you to create gRPC status responses with structured error
/// information conforming to the Google RPC error model. It supports adding
/// multiple error details of various types to provide comprehensive error
/// information to clients.
///
/// @see https:///google.aip.dev/193
/// @see https:///grpc.io/docs/guides/error/
/// @see @ref ugrpc::ErrorInfo
/// @see @ref ugrpc::RetryInfo
/// @see @ref ugrpc::DebugInfo
/// @see @ref ugrpc::QuotaFailure
/// @see @ref ugrpc::PreconditionFailure
/// @see @ref ugrpc::BadRequest
/// @see @ref ugrpc::RequestInfo
/// @see @ref ugrpc::ResourceInfo
/// @see @ref ugrpc::Help
/// @see @ref ugrpc::LocalizedMessage
///
/// ## Example usage:
/// @snippet grpc/tests/rich_status_test.cpp rich_status_usage
class RichStatus final {
public:
    /// @brief Constructs an `OK` status with no error details.
    RichStatus() = default;

    /// @brief Constructs a `RichStatus` from a `grpc::Status`.
    /// @param grpc_status The gRPC status to convert. If it contains serialized
    ///                    `google::rpc::Status` in `error_details()`, it will be
    ///                    parsed and used.
    explicit RichStatus(const grpc::Status& grpc_status);

    /// @brief Constructs a `RichStatus` with the given code, message, and details.
    /// @param code The gRPC status code
    /// @param message The error message
    /// @param details Error details conforming to `google.rpc.error_details` types.
    /// Must have a `ToGoogleErrorDetail()` method.
    template <typename... TDetails>
    RichStatus(grpc::StatusCode code, std::string message, TDetails&&... details);

    /// @brief Adds an error detail to the status.
    /// @param detail The error detail to add. Must have a `ToGoogleErrorDetail()` method.
    /// @return Reference to this `RichStatus` for method chaining.
    template <typename TDetail>
    RichStatus& AddDetail(TDetail&& detail);

    /// @brief Attempts to extract a specific rich error detail from `google::rpc::Status`
    /// @return The extracted rich error detail if found, `std::nullopt` otherwise
    ///
    /// According to AIP-193 (https://google.aip.dev/193), each status
    /// should contain at most one detail of each type. If multiple details
    /// of the same type exist (violating AIP-193), this function will
    /// return only the first encountered instance.
    ///
    /// ## Example usage:
    /// @snippet grpc/tests/rich_status_test.cpp try_get_detail_example
    template <typename TRichErrorDetail>
    [[nodiscard]] static std::optional<TRichErrorDetail> TryGetDetail(const google::rpc::Status& status);

    [[nodiscard]] const google::rpc::Status& GetGoogleStatus() const& { return google_status_; }
    google::rpc::Status GetGoogleStatus() && { return std::move(google_status_); }

    /// @brief Converts this `RichStatus` to a `grpc::Status`.
    /// @return A `grpc::Status` with serialized error details.
    [[nodiscard]] grpc::Status ToGrpcStatus() const;

    [[nodiscard]] explicit operator grpc::Status() const { return ToGrpcStatus(); }

private:
    google::rpc::Status google_status_;
};

/// @brief Provides structured error information about the cause of an error.
///
/// @see https:///google.aip.dev/193
/// @see https:///github.com/googleapis/googleapis/blob/master/google/rpc/error_details.proto
/// @see @ref ugrpc::RichStatus
///
/// Use this detail to provide a machine-readable error reason, domain, and
/// additional metadata. This is useful for error handling logic on the client side.
///
/// Any request-specific information which contributes to the `Status.message` or `LocalizedMessage.message` messages
/// must be represented within metadata. This practice is critical so that machine actors do not need to parse error
/// messages to extract information.
///
/// ## Example usage:
/// @snippet grpc/tests/rich_status_test.cpp error_info_example
struct ErrorInfo {
    /// @brief The reason of the error (e.g., "INVALID_TOKEN", "SERVICE_DISABLED").
    ///
    /// The reason field is a short snake_case description of the cause of the error. Error reasons are unique within a
    /// particular domain of errors. The reason must be at most 63 characters and match a regular expression of
    /// `[A-Z][A-Z0-9_]+[A-Z0-9]`. (This is UPPER_SNAKE_CASE, without leading or trailing underscores, and without
    /// leading digits.)
    std::string reason;

    /// @brief The logical grouping to which the error belongs (e.g., "auth.example.com").
    std::string domain;

    /// @brief Additional structured details about the error.
    std::unordered_map<std::string, std::string> metadata;

    google::rpc::ErrorInfo ToGoogleErrorDetail() const&;
    google::rpc::ErrorInfo ToGoogleErrorDetail() &&;

    static std::optional<ErrorInfo> TryUnpack(const google::protobuf::Any& any);
};

/// @brief Describes when the client can retry a failed request.
///
/// @see https:///github.com/googleapis/googleapis/blob/master/google/rpc/error_details.proto
/// @see @ref ugrpc::RichStatus
///
/// ## Example usage:
/// @snippet grpc/tests/rich_status_test.cpp retry_info_example
struct RetryInfo {
    /// @brief Clients should wait at least this long between retrying the same request.
    std::chrono::nanoseconds retry_delay;

    google::rpc::RetryInfo ToGoogleErrorDetail() const;

    static std::optional<RetryInfo> TryUnpack(const google::protobuf::Any& any);
};

/// @brief Provides debugging information such as stack traces.
///
/// @see @ref ugrpc::RichStatus
///
/// ## Example usage:
/// @snippet grpc/tests/rich_status_test.cpp debug_info_example
struct DebugInfo {
    std::vector<std::string> stack_entries;
    std::string detail;

    google::rpc::DebugInfo ToGoogleErrorDetail() const&;
    google::rpc::DebugInfo ToGoogleErrorDetail() &&;

    static std::optional<DebugInfo> TryUnpack(const google::protobuf::Any& any);
};

/// @brief Describes a single quota violation.
///
/// @see @ref ugrpc::QuotaFailure
struct QuotaViolation {
    /// @brief The subject on which the quota check failed.
    ///
    /// Example: "clientip:<ip address>", "project:<project id>"
    std::string subject;

    /// @brief A description of how the quota check failed.
    std::string description;
};

/// @brief Describes quota violations that caused the request to fail.
///
/// @see https:///github.com/googleapis/googleapis/blob/master/google/rpc/error_details.proto
/// @see @ref ugrpc::RichStatus
///
/// ## Example usage:
/// @snippet grpc/tests/rich_status_test.cpp quota_failure_example
struct QuotaFailure {
    std::vector<QuotaViolation> violations;

    google::rpc::QuotaFailure ToGoogleErrorDetail() const&;
    google::rpc::QuotaFailure ToGoogleErrorDetail() &&;

    static std::optional<QuotaFailure> TryUnpack(const google::protobuf::Any& any);
};

/// @brief Describes a single precondition violation.
///
/// @see @ref ugrpc::PreconditionFailure
struct PreconditionViolation {
    /// @brief The type of precondition that failed (e.g., "TOS", "AGE").
    ///
    /// Google recommends using a service-specific enum type to define the supported
    /// precondition violation subjects. For example, "TOS" for "Terms of Service violation".
    std::string type;

    /// @brief The subject on which the precondition failed (e.g., "user:123").
    ///
    /// For example, "google.com/cloud" relative to the "TOS" type would indicate
    /// which terms of service is being referenced.
    std::string subject;

    /// @brief A description of how the precondition failed.
    ///
    /// Developers can use this description to understand how to fix the failure.
    /// For Example usage: "Terms of service not accepted".
    std::string description;
};

/// @brief Describes preconditions that failed during request processing.
///
/// @see https:///github.com/googleapis/googleapis/blob/master/google/rpc/error_details.proto
/// @see @ref ugrpc::RichStatus
///
/// For example, if an RPC failed because it required the Terms of Service to be
/// acknowledged, it could list the terms of service violation in the
/// PreconditionFailure message.
///
/// ## Example usage:
/// @snippet grpc/tests/rich_status_test.cpp precondition_failure_example
struct PreconditionFailure {
    /// @brief The list of precondition violations.
    std::vector<PreconditionViolation> violations;

    google::rpc::PreconditionFailure ToGoogleErrorDetail() const&;
    google::rpc::PreconditionFailure ToGoogleErrorDetail() &&;

    static std::optional<PreconditionFailure> TryUnpack(const google::protobuf::Any& any);
};

/// @brief Describes a single field validation error.
///
/// @see @ref ugrpc::BadRequest
struct FieldViolation {
    /// @brief The field path (e.g., "user.email", "address.postal_code").
    std::string field;

    /// @brief A description of why the field is invalid.
    std::string description;
};

/// @brief Describes violations in a client request. This error type focuses on the
/// syntactic aspects of the request.
///
/// @see https:///github.com/googleapis/googleapis/blob/master/google/rpc/error_details.proto
/// @see @ref ugrpc::RichStatus
///
/// ## Example usage:
/// @snippet grpc/tests/rich_status_test.cpp bad_request_example
struct BadRequest {
    std::vector<FieldViolation> field_violations;

    google::rpc::BadRequest ToGoogleErrorDetail() const&;
    google::rpc::BadRequest ToGoogleErrorDetail() &&;

    static std::optional<BadRequest> TryUnpack(const google::protobuf::Any& any);
};

/// @brief Contains metadata about the request for debugging and logging.
///
/// @see https:///github.com/googleapis/googleapis/blob/master/google/rpc/error_details.proto
/// @see @ref ugrpc::RichStatus
///
///
/// Use this detail to provide request identification information that can
/// help with debugging and tracking requests across services.
///
/// ## Example usage:
/// @snippet grpc/tests/rich_status_test.cpp request_info_example
struct RequestInfo {
    std::string request_id;
    std::string serving_data;

    google::rpc::RequestInfo ToGoogleErrorDetail() const&;
    google::rpc::RequestInfo ToGoogleErrorDetail() &&;

    static std::optional<RequestInfo> TryUnpack(const google::protobuf::Any& any);
};

/// @brief Provides information about a resource that is related to the error.
///
/// @see https:///github.com/googleapis/googleapis/blob/master/google/rpc/error_details.proto
/// @see @ref ugrpc::RichStatus
///
/// Use this detail to describe a resource that caused the error or is
/// affected by the error. Commonly used with `NOT_FOUND`, `ALREADY_EXISTS`,
/// or `PERMISSION_DENIED` status codes.
///
/// ## Example usage:
/// @snippet grpc/tests/rich_status_test.cpp resource_info_example
struct ResourceInfo {
    /// @brief The type of resource (e.g., "storage.bucket", "compute.instance").
    std::string resource_type;

    /// @brief The name/identifier of the resource.
    std::string resource_name;

    /// @brief The owner of the resource.
    std::string owner;

    /// @brief The description of the error that is encountered when accessing this resource.
    std::string description;

    google::rpc::ResourceInfo ToGoogleErrorDetail() const&;
    google::rpc::ResourceInfo ToGoogleErrorDetail() &&;

    static std::optional<ResourceInfo> TryUnpack(const google::protobuf::Any& any);
};

/// @brief Describes a single help link.
///
/// @see @ref ugrpc::Help
struct HelpLink {
    std::string description;
    std::string url;
};

/// @brief Provides links to documentation and help resources.
///
/// @see https://google.aip.dev/193
/// @see https:///github.com/googleapis/googleapis/blob/master/google/rpc/error_details.proto
/// @see https://nda.ya.ru/t/PmNiceWp7NKDXb
/// @see @ref ugrpc::RichStatus
///
/// Use this detail to direct users to relevant documentation, FAQs,
/// or support resources that can help them resolve the error.
///
/// ## Example usage:
/// @snippet grpc/tests/rich_status_test.cpp help_example
struct Help {
    /// @brief The list of help links.
    std::vector<HelpLink> links;

    google::rpc::Help ToGoogleErrorDetail() const&;
    google::rpc::Help ToGoogleErrorDetail() &&;

    static std::optional<Help> TryUnpack(const google::protobuf::Any& any);
};

/// @brief Provides a localized error message that is safe to return to the user.
///
/// @see https://google.aip.dev/193
/// @see https:///github.com/googleapis/googleapis/blob/master/google/rpc/error_details.proto
/// @see @ref ugrpc::RichStatus
///
/// Provides a localized error message that is safe to return to the user
/// which can be attached to an RPC error.
///
/// The `LocalizedMessage` payload should contain the complete resolution to the error. If more information is needed
/// than can reasonably fit in this payload, then additional resolution information must be provided in a @ref Help
/// payload.
///
/// ## Example usage:
/// @snippet grpc/tests/rich_status_test.cpp localized_message_example
struct LocalizedMessage {
    /// @brief The locale (e.g., "en-US", "ru-RU", "ja-JP").
    std::string locale;

    /// @brief The localized error message.
    ///
    /// This should include a brief description of the error and a call to action to resolve the error. The message
    /// should include contextual information to make the message as specific as possible. Any contextual information in
    /// the message must be included in @ref ErrorInfo.metadata.
    std::string message;

    google::rpc::LocalizedMessage ToGoogleErrorDetail() const&;
    google::rpc::LocalizedMessage ToGoogleErrorDetail() &&;

    static std::optional<LocalizedMessage> TryUnpack(const google::protobuf::Any& any);
};

template <typename... TDetails>
RichStatus::RichStatus(grpc::StatusCode code, std::string message, TDetails&&... details) {
    google_status_.set_code(static_cast<int>(code));
    google_status_.set_message(std::move(message));
    (AddDetail(std::forward<TDetails>(details)), ...);
}

template <typename TDetail>
RichStatus& RichStatus::AddDetail(TDetail&& detail) {
    const auto pb_google_detail = std::forward<TDetail>(detail).ToGoogleErrorDetail();
    google_status_.add_details()->PackFrom(pb_google_detail);
    return *this;
}

template <typename TRichErrorDetail>
std::optional<TRichErrorDetail> RichStatus::TryGetDetail(const google::rpc::Status& status) {
    for (const auto& detail : status.details()) {
        const auto rich_error_detail_opt = TRichErrorDetail::TryUnpack(detail);
        if (rich_error_detail_opt) {
            return *rich_error_detail_opt;
        }
    }
    return std::nullopt;
}

}  // namespace ugrpc

USERVER_NAMESPACE_END
