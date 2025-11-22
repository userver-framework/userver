#include <userver/utest/utest.hpp>

#include <google/rpc/error_details.pb.h>
#include <google/rpc/status.pb.h>

#include <userver/ugrpc/rich_status.hpp>
#include <userver/ugrpc/status_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

void CheckCommonFields(
    const google::rpc::Status& google_status,
    grpc::StatusCode expected_code,
    const std::string& expected_message,
    int expected_details_count
) {
    EXPECT_EQ(google_status.code(), static_cast<int>(expected_code));
    EXPECT_EQ(google_status.message(), expected_message);
    EXPECT_EQ(google_status.details_size(), expected_details_count);
}

}  // namespace

UTEST(RichStatus, DefaultConstructor) {
    ugrpc::RichStatus status;

    const auto google_status = std::move(status).GetGoogleStatus();

    EXPECT_EQ(google_status.code(), grpc::StatusCode::OK);
    EXPECT_TRUE(google_status.message().empty());
    EXPECT_EQ(google_status.details_size(), 0);
}

UTEST(RichStatus, FromGrpcStatus) {
    const auto code = grpc::StatusCode::NOT_FOUND;
    const auto message = "not found";

    grpc::Status grpc_status{code, message};
    ugrpc::RichStatus status{grpc_status};

    const auto google_status = std::move(status).GetGoogleStatus();

    CheckCommonFields(google_status, code, message, 0);
}

UTEST(RichStatus, ConstructorWithMessage) {
    const auto code = grpc::StatusCode::INVALID_ARGUMENT;
    const auto message = "invalid argument provided";

    ugrpc::RichStatus status{code, message};

    const auto google_status = std::move(status).GetGoogleStatus();

    CheckCommonFields(google_status, code, message, 0);
}

UTEST(RichStatus, ToGrpcStatus) {
    const auto code = grpc::StatusCode::INTERNAL;
    const auto message = "internal error";

    ugrpc::RichStatus status{code, message};
    grpc::Status grpc_status = status.ToGrpcStatus();

    EXPECT_EQ(grpc_status.error_code(), code);
    EXPECT_EQ(grpc_status.error_message(), message);
}

UTEST(RichStatus, ErrorInfo) {
    /// [error_info_example]
    ugrpc::RichStatus status{
        grpc::StatusCode::UNAUTHENTICATED,
        "Authentication failed",
        ugrpc::ErrorInfo{
            "INVALID_TOKEN",
            "auth.example.com",
            {{"token_type", "bearer"}, {"error", "expired"}},
        },
    };
    /// [error_info_example]

    const auto google_status = std::move(status).GetGoogleStatus();

    CheckCommonFields(google_status, grpc::StatusCode::UNAUTHENTICATED, "Authentication failed", 1);

    google::rpc::ErrorInfo unpacked;
    EXPECT_TRUE(google_status.details(0).UnpackTo(&unpacked));
    EXPECT_EQ(unpacked.reason(), "INVALID_TOKEN");
    EXPECT_EQ(unpacked.domain(), "auth.example.com");
    EXPECT_EQ(unpacked.metadata().size(), 2);
    EXPECT_EQ(unpacked.metadata().at("token_type"), "bearer");
    EXPECT_EQ(unpacked.metadata().at("error"), "expired");

    auto unpacked_error_info_opt = ugrpc::ErrorInfo::TryUnpack(google_status.details(0));
    EXPECT_TRUE(unpacked_error_info_opt.has_value());
    const auto& unpacked_rich = *unpacked_error_info_opt;
    EXPECT_EQ(unpacked_rich.reason, "INVALID_TOKEN");
    EXPECT_EQ(unpacked_rich.domain, "auth.example.com");
    EXPECT_EQ(unpacked_rich.metadata.at("token_type"), "bearer");
    EXPECT_EQ(unpacked_rich.metadata.at("error"), "expired");
}

UTEST(RichStatus, RetryInfo) {
    /// [retry_info_example]
    ugrpc::RichStatus status{
        grpc::StatusCode::UNAVAILABLE,
        "Service overloaded",
        ugrpc::RetryInfo{std::chrono::minutes{5}},
    };
    /// [retry_info_example]

    const auto google_status = std::move(status).GetGoogleStatus();

    CheckCommonFields(google_status, grpc::StatusCode::UNAVAILABLE, "Service overloaded", 1);

    google::rpc::RetryInfo unpacked;
    EXPECT_TRUE(google_status.details(0).UnpackTo(&unpacked));
    EXPECT_EQ(unpacked.retry_delay().seconds(), 300);  // 5 minutes
    EXPECT_EQ(unpacked.retry_delay().nanos(), 0);

    auto unpacked_retry_info_opt = ugrpc::RetryInfo::TryUnpack(google_status.details(0));
    EXPECT_TRUE(unpacked_retry_info_opt.has_value());
    EXPECT_EQ(unpacked_retry_info_opt->retry_delay, std::chrono::minutes{5});
}

UTEST(RichStatus, DebugInfo) {
    /// [debug_info_example]
    ugrpc::RichStatus status{
        grpc::StatusCode::INTERNAL,
        "Unexpected error",
        ugrpc::DebugInfo{
            {
                "at Handler::Process() in handler.cpp:42",
                "at main() in main.cpp:10",
            },
            "Null pointer dereference in request processing",
        },
    };
    /// [debug_info_example]

    const auto google_status = std::move(status).GetGoogleStatus();

    CheckCommonFields(google_status, grpc::StatusCode::INTERNAL, "Unexpected error", 1);

    google::rpc::DebugInfo unpacked;
    EXPECT_TRUE(google_status.details(0).UnpackTo(&unpacked));
    EXPECT_EQ(unpacked.stack_entries_size(), 2);
    EXPECT_EQ(unpacked.stack_entries(0), "at Handler::Process() in handler.cpp:42");
    EXPECT_EQ(unpacked.stack_entries(1), "at main() in main.cpp:10");
    EXPECT_EQ(unpacked.detail(), "Null pointer dereference in request processing");

    auto unpacked_debug_info_opt = ugrpc::DebugInfo::TryUnpack(google_status.details(0));
    EXPECT_TRUE(unpacked_debug_info_opt.has_value());
    const auto& unpacked_rich = *unpacked_debug_info_opt;
    EXPECT_EQ(unpacked_rich.stack_entries.size(), 2);
    EXPECT_EQ(unpacked_rich.stack_entries[0], "at Handler::Process() in handler.cpp:42");
    EXPECT_EQ(unpacked_rich.stack_entries[1], "at main() in main.cpp:10");
    EXPECT_EQ(unpacked_rich.detail, "Null pointer dereference in request processing");
}

UTEST(RichStatus, QuotaFailure) {
    /// [quota_failure_example]
    ugrpc::RichStatus status{
        grpc::StatusCode::RESOURCE_EXHAUSTED,
        "Quota exceeded",
        ugrpc::QuotaFailure{{
            {"user:alice@example.com", "Daily request quota exceeded"},
            {"project:example-project", "Rate limit of 100 req/s exceeded"},
        }},
    };
    /// [quota_failure_example]

    const auto google_status = std::move(status).GetGoogleStatus();

    CheckCommonFields(google_status, grpc::StatusCode::RESOURCE_EXHAUSTED, "Quota exceeded", 1);

    google::rpc::QuotaFailure unpacked;
    EXPECT_TRUE(google_status.details(0).UnpackTo(&unpacked));
    EXPECT_EQ(unpacked.violations_size(), 2);
    EXPECT_EQ(unpacked.violations(0).subject(), "user:alice@example.com");
    EXPECT_EQ(unpacked.violations(0).description(), "Daily request quota exceeded");
    EXPECT_EQ(unpacked.violations(1).subject(), "project:example-project");
    EXPECT_EQ(unpacked.violations(1).description(), "Rate limit of 100 req/s exceeded");

    auto unpacked_quota_failure_opt = ugrpc::QuotaFailure::TryUnpack(google_status.details(0));
    EXPECT_TRUE(unpacked_quota_failure_opt.has_value());
    const auto& unpacked_rich = *unpacked_quota_failure_opt;
    EXPECT_EQ(unpacked_rich.violations.size(), 2);
    EXPECT_EQ(unpacked_rich.violations[0].subject, "user:alice@example.com");
    EXPECT_EQ(unpacked_rich.violations[0].description, "Daily request quota exceeded");
    EXPECT_EQ(unpacked_rich.violations[1].subject, "project:example-project");
    EXPECT_EQ(unpacked_rich.violations[1].description, "Rate limit of 100 req/s exceeded");
}

UTEST(RichStatus, PreconditionFailure) {
    /// [precondition_failure_example]
    ugrpc::RichStatus status{
        grpc::StatusCode::FAILED_PRECONDITION,
        "Preconditions not met",
        ugrpc::PreconditionFailure{{
            {"TOS", "user:alice@example.com", "Terms of service not accepted"},
            {"EMAIL_VERIFIED", "user:alice@example.com", "Email not verified"},
        }},
    };
    /// [precondition_failure_example]

    const auto google_status = std::move(status).GetGoogleStatus();

    CheckCommonFields(google_status, grpc::StatusCode::FAILED_PRECONDITION, "Preconditions not met", 1);

    google::rpc::PreconditionFailure unpacked;
    EXPECT_TRUE(google_status.details(0).UnpackTo(&unpacked));
    EXPECT_EQ(unpacked.violations_size(), 2);
    EXPECT_EQ(unpacked.violations(0).type(), "TOS");
    EXPECT_EQ(unpacked.violations(0).subject(), "user:alice@example.com");
    EXPECT_EQ(unpacked.violations(0).description(), "Terms of service not accepted");
    EXPECT_EQ(unpacked.violations(1).type(), "EMAIL_VERIFIED");
    EXPECT_EQ(unpacked.violations(1).subject(), "user:alice@example.com");
    EXPECT_EQ(unpacked.violations(1).description(), "Email not verified");

    auto unpacked_precondition_failure_opt = ugrpc::PreconditionFailure::TryUnpack(google_status.details(0));
    EXPECT_TRUE(unpacked_precondition_failure_opt.has_value());
    const auto& unpacked_rich = *unpacked_precondition_failure_opt;
    EXPECT_EQ(unpacked_rich.violations.size(), 2);
    EXPECT_EQ(unpacked_rich.violations[0].type, "TOS");
    EXPECT_EQ(unpacked_rich.violations[0].subject, "user:alice@example.com");
    EXPECT_EQ(unpacked_rich.violations[0].description, "Terms of service not accepted");
    EXPECT_EQ(unpacked_rich.violations[1].type, "EMAIL_VERIFIED");
    EXPECT_EQ(unpacked_rich.violations[1].subject, "user:alice@example.com");
    EXPECT_EQ(unpacked_rich.violations[1].description, "Email not verified");
}

UTEST(RichStatus, BadRequest) {
    /// [bad_request_example]
    ugrpc::RichStatus status{
        grpc::StatusCode::INVALID_ARGUMENT,
        "Invalid request",
        ugrpc::BadRequest{{
            {"user.email", "Invalid email format"},
            {"user.age", "Must be between 18 and 120"},
            {"user.name", "Required field is missing"},
        }},
    };
    /// [bad_request_example]

    const auto google_status = std::move(status).GetGoogleStatus();

    CheckCommonFields(google_status, grpc::StatusCode::INVALID_ARGUMENT, "Invalid request", 1);

    google::rpc::BadRequest unpacked;
    EXPECT_TRUE(google_status.details(0).UnpackTo(&unpacked));
    EXPECT_EQ(unpacked.field_violations_size(), 3);
    EXPECT_EQ(unpacked.field_violations(0).field(), "user.email");
    EXPECT_EQ(unpacked.field_violations(0).description(), "Invalid email format");
    EXPECT_EQ(unpacked.field_violations(1).field(), "user.age");
    EXPECT_EQ(unpacked.field_violations(1).description(), "Must be between 18 and 120");
    EXPECT_EQ(unpacked.field_violations(2).field(), "user.name");
    EXPECT_EQ(unpacked.field_violations(2).description(), "Required field is missing");

    auto unpacked_bad_request_opt = ugrpc::BadRequest::TryUnpack(google_status.details(0));
    EXPECT_TRUE(unpacked_bad_request_opt.has_value());
    const auto& unpacked_rich = *unpacked_bad_request_opt;
    EXPECT_EQ(unpacked_rich.field_violations.size(), 3);
    EXPECT_EQ(unpacked_rich.field_violations[0].field, "user.email");
    EXPECT_EQ(unpacked_rich.field_violations[0].description, "Invalid email format");
    EXPECT_EQ(unpacked_rich.field_violations[1].field, "user.age");
    EXPECT_EQ(unpacked_rich.field_violations[1].description, "Must be between 18 and 120");
    EXPECT_EQ(unpacked_rich.field_violations[2].field, "user.name");
    EXPECT_EQ(unpacked_rich.field_violations[2].description, "Required field is missing");
}

UTEST(RichStatus, RequestInfo) {
    /// [request_info_example]
    ugrpc::RichStatus status{
        grpc::StatusCode::INTERNAL,
        "Internal server error",
        ugrpc::RequestInfo{"req-20231215-abc123", "backend-server-42"},
    };
    /// [request_info_example]

    const auto google_status = std::move(status).GetGoogleStatus();

    CheckCommonFields(google_status, grpc::StatusCode::INTERNAL, "Internal server error", 1);

    google::rpc::RequestInfo unpacked;
    EXPECT_TRUE(google_status.details(0).UnpackTo(&unpacked));
    EXPECT_EQ(unpacked.request_id(), "req-20231215-abc123");
    EXPECT_EQ(unpacked.serving_data(), "backend-server-42");

    auto unpacked_request_info_opt = ugrpc::RequestInfo::TryUnpack(google_status.details(0));
    EXPECT_TRUE(unpacked_request_info_opt.has_value());
    const auto& unpacked_rich = *unpacked_request_info_opt;
    EXPECT_EQ(unpacked_rich.request_id, "req-20231215-abc123");
    EXPECT_EQ(unpacked_rich.serving_data, "backend-server-42");
}

UTEST(RichStatus, ResourceInfo) {
    /// [resource_info_example]
    ugrpc::RichStatus status{
        grpc::StatusCode::NOT_FOUND,
        "Resource not found",
        ugrpc::ResourceInfo{
            "storage.bucket",
            "projects/my-project/buckets/my-bucket",
            "user:alice@example.com",
            "Private user bucket",
        },
    };
    /// [resource_info_example]

    const auto google_status = std::move(status).GetGoogleStatus();

    CheckCommonFields(google_status, grpc::StatusCode::NOT_FOUND, "Resource not found", 1);

    google::rpc::ResourceInfo unpacked;
    EXPECT_TRUE(google_status.details(0).UnpackTo(&unpacked));
    EXPECT_EQ(unpacked.resource_type(), "storage.bucket");
    EXPECT_EQ(unpacked.resource_name(), "projects/my-project/buckets/my-bucket");
    EXPECT_EQ(unpacked.owner(), "user:alice@example.com");
    EXPECT_EQ(unpacked.description(), "Private user bucket");

    auto unpacked_resource_info_opt = ugrpc::ResourceInfo::TryUnpack(google_status.details(0));
    EXPECT_TRUE(unpacked_resource_info_opt.has_value());
    const auto& unpacked_rich = *unpacked_resource_info_opt;
    EXPECT_EQ(unpacked_rich.resource_type, "storage.bucket");
    EXPECT_EQ(unpacked_rich.resource_name, "projects/my-project/buckets/my-bucket");
    EXPECT_EQ(unpacked_rich.owner, "user:alice@example.com");
    EXPECT_EQ(unpacked_rich.description, "Private user bucket");
}

UTEST(RichStatus, Help) {
    /// [help_example]
    ugrpc::RichStatus status{
        grpc::StatusCode::PERMISSION_DENIED,
        "Access denied",
        ugrpc::Help{{
            {"API Documentation", "https://example.com/docs/api"},
            {"Authentication Guide", "https://example.com/docs/auth"},
            {"Support", "https://example.com/support"},
        }},
    };
    /// [help_example]

    const auto google_status = std::move(status).GetGoogleStatus();

    CheckCommonFields(google_status, grpc::StatusCode::PERMISSION_DENIED, "Access denied", 1);

    google::rpc::Help unpacked;
    EXPECT_TRUE(google_status.details(0).UnpackTo(&unpacked));
    EXPECT_EQ(unpacked.links_size(), 3);
    EXPECT_EQ(unpacked.links(0).description(), "API Documentation");
    EXPECT_EQ(unpacked.links(0).url(), "https://example.com/docs/api");
    EXPECT_EQ(unpacked.links(1).description(), "Authentication Guide");
    EXPECT_EQ(unpacked.links(1).url(), "https://example.com/docs/auth");
    EXPECT_EQ(unpacked.links(2).description(), "Support");
    EXPECT_EQ(unpacked.links(2).url(), "https://example.com/support");

    auto unpacked_help_opt = ugrpc::Help::TryUnpack(google_status.details(0));
    EXPECT_TRUE(unpacked_help_opt.has_value());
    const auto& unpacked_rich = *unpacked_help_opt;
    EXPECT_EQ(unpacked_rich.links.size(), 3);
    EXPECT_EQ(unpacked_rich.links[0].description, "API Documentation");
    EXPECT_EQ(unpacked_rich.links[0].url, "https://example.com/docs/api");
    EXPECT_EQ(unpacked_rich.links[1].description, "Authentication Guide");
    EXPECT_EQ(unpacked_rich.links[1].url, "https://example.com/docs/auth");
    EXPECT_EQ(unpacked_rich.links[2].description, "Support");
    EXPECT_EQ(unpacked_rich.links[2].url, "https://example.com/support");
}

UTEST(RichStatus, LocalizedMessage) {
    /// [localized_message_example]
    ugrpc::RichStatus status{
        grpc::StatusCode::INVALID_ARGUMENT,
        "Invalid email address",
        ugrpc::LocalizedMessage{
            "ru-RU",
            "Неверный формат электронной почты. Пожалуйста, используйте name@example.com",
        },
    };
    /// [localized_message_example]

    const auto google_status = std::move(status).GetGoogleStatus();

    CheckCommonFields(google_status, grpc::StatusCode::INVALID_ARGUMENT, "Invalid email address", 1);

    google::rpc::LocalizedMessage unpacked;
    EXPECT_TRUE(google_status.details(0).UnpackTo(&unpacked));
    EXPECT_EQ(unpacked.locale(), "ru-RU");
    EXPECT_EQ(unpacked.message(), "Неверный формат электронной почты. Пожалуйста, используйте name@example.com");

    auto unpacked_localized_message_opt = ugrpc::LocalizedMessage::TryUnpack(google_status.details(0));
    EXPECT_TRUE(unpacked_localized_message_opt.has_value());
    const auto& unpacked_rich = *unpacked_localized_message_opt;
    EXPECT_EQ(unpacked_rich.locale, "ru-RU");
    EXPECT_EQ(unpacked_rich.message, "Неверный формат электронной почты. Пожалуйста, используйте name@example.com");
}

UTEST(RichStatus, MultipleDetails) {
    ugrpc::RichStatus status{
        grpc::StatusCode::UNAVAILABLE,
        "Service unavailable",
        ugrpc::RetryInfo{std::chrono::seconds{5}},
        ugrpc::DebugInfo{{"stack trace"}, "error info"},
        ugrpc::Help{{{"Docs", "https://docs.com"}}},
    };

    const auto google_status = std::move(status).GetGoogleStatus();

    CheckCommonFields(google_status, grpc::StatusCode::UNAVAILABLE, "Service unavailable", 3);

    google::rpc::RetryInfo unpacked_retry;
    EXPECT_TRUE(google_status.details(0).UnpackTo(&unpacked_retry));

    google::rpc::DebugInfo unpacked_debug;
    EXPECT_TRUE(google_status.details(1).UnpackTo(&unpacked_debug));

    google::rpc::Help unpacked_help;
    EXPECT_TRUE(google_status.details(2).UnpackTo(&unpacked_help));
}

UTEST(RichStatus, AddDetailChaining) {
    const auto code = grpc::StatusCode::OUT_OF_RANGE;
    const auto message = "value out of range";

    ugrpc::RichStatus status{code, message};
    status.AddDetail(ugrpc::RetryInfo{std::chrono::seconds{1}}).AddDetail(ugrpc::DebugInfo{{"line1"}, "detail"});

    const auto google_status = std::move(status).GetGoogleStatus();

    CheckCommonFields(google_status, code, message, 2);

    google::rpc::RetryInfo unpacked_retry;
    EXPECT_TRUE(google_status.details(0).UnpackTo(&unpacked_retry));

    google::rpc::DebugInfo unpacked_debug;
    EXPECT_TRUE(google_status.details(1).UnpackTo(&unpacked_debug));
}

UTEST(RichStatus, RoundTripConversion) {
    const auto code = grpc::StatusCode::PERMISSION_DENIED;
    const auto message = "denied";

    ugrpc::RichStatus original{
        code,
        message,
        ugrpc::ErrorInfo{
            "ERROR",
            "domain",
            {{"key", "value"}},
        },
    };

    const grpc::Status grpc_status = original.ToGrpcStatus();
    const ugrpc::RichStatus restored{grpc_status};
    const grpc::Status restored_grpc_status = restored.ToGrpcStatus();

    EXPECT_EQ(grpc_status.error_code(), restored_grpc_status.error_code());
    EXPECT_EQ(grpc_status.error_message(), restored_grpc_status.error_message());
    EXPECT_EQ(grpc_status.error_details(), restored_grpc_status.error_details());

    const auto google_status = std::move(restored).GetGoogleStatus();

    EXPECT_EQ(google_status.code(), static_cast<int>(code));
    EXPECT_EQ(google_status.message(), message);
    EXPECT_EQ(google_status.details_size(), 1);

    google::rpc::ErrorInfo unpacked;
    EXPECT_TRUE(google_status.details(0).UnpackTo(&unpacked));
    EXPECT_EQ(unpacked.reason(), "ERROR");
    EXPECT_EQ(unpacked.domain(), "domain");
    EXPECT_EQ(unpacked.metadata().at("key"), "value");
}

USERVER_NAMESPACE_END
