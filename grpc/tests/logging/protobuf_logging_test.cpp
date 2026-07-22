#include <userver/ugrpc/protobuf_logging.hpp>

#include <gmock/gmock.h>
#include <google/protobuf/empty.pb.h>
#include <google/protobuf/wrappers.pb.h>
#include <google/rpc/status.pb.h>
#include <grpcpp/support/status.h>

#include <userver/logging/log.hpp>
#include <userver/utest/log_capture_fixture.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

static constexpr std::string_view kTruncateMarker = "...(truncated)";

UTEST(ProtobufLogging, GetMessageWithData) {
    google::protobuf::StringValue test_message;
    test_message.set_value("test string with some content");

    const auto result = ugrpc::ToLimitedLoggingString(test_message, 1000);

    EXPECT_THAT(result, testing::HasSubstr("test string with some content"));
    EXPECT_THAT(result, testing::Not(testing::HasSubstr(kTruncateMarker)));
}

UTEST(ProtobufLogging, GetMessageEmptyMessage) {
    // Unlike the debug string, ProtoJSON of a message with no fields is a valid '{}', not an empty string.
    google::protobuf::Empty empty_message;
    const auto result = ugrpc::ToLimitedLoggingString(empty_message, 100);

    EXPECT_EQ(result, "{}");
}

UTEST(ProtobufLogging, GetMessageSizeLimit) {
    google::protobuf::StringValue test_message;
    test_message.set_value("test string with some content");

    const auto small_result = ugrpc::ToLimitedLoggingString(test_message, 10);
    const auto large_result = ugrpc::ToLimitedLoggingString(test_message, 1000);

    EXPECT_THAT(small_result, testing::EndsWith(kTruncateMarker));
    EXPECT_GT(large_result.size(), small_result.size());
    EXPECT_THAT(large_result, testing::HasSubstr("test string with some content"));
    EXPECT_THAT(large_result, testing::Not(testing::HasSubstr(kTruncateMarker)));
}

UTEST(ProtobufLogging, OkStatus) {
    grpc::Status ok_status = grpc::Status::OK;
    const auto result = ugrpc::ToUnlimitedLoggingString(ok_status);

    EXPECT_EQ(result, R"({"code":"OK"})");
}

UTEST(ProtobufLogging, SimpleError) {
    grpc::Status simple_error(grpc::StatusCode::NOT_FOUND, "Resource not found");
    const auto result = ugrpc::ToUnlimitedLoggingString(simple_error);

    EXPECT_THAT(result, testing::HasSubstr("NOT_FOUND"));
    EXPECT_THAT(result, testing::HasSubstr("Resource not found"));
}

UTEST(ProtobufLogging, ComplexError) {
    google::rpc::Status error_details;
    error_details.set_code(static_cast<int>(grpc::StatusCode::INVALID_ARGUMENT));
    error_details.set_message("Invalid parameter provided");

    auto* detail = error_details.add_details();
    detail->set_type_url("type.googleapis.com/google.protobuf.StringValue");
    google::protobuf::StringValue detail_value;
    detail_value.set_value("Additional error context");
    detail->set_value(detail_value.SerializeAsString());

    grpc::Status complex_status(
        grpc::StatusCode::INVALID_ARGUMENT,
        "Invalid parameter provided",
        error_details.SerializeAsString()
    );

    const auto result = ugrpc::ToUnlimitedLoggingString(complex_status);

    EXPECT_THAT(result, testing::HasSubstr("INVALID_ARGUMENT"));
    EXPECT_THAT(result, testing::HasSubstr("Invalid parameter provided"));
    EXPECT_THAT(result, testing::HasSubstr("Additional error context"));
    EXPECT_THAT(result, testing::HasSubstr("type.googleapis.com"));
}

UTEST(ProtobufLogging, EdgeCasesZeroMaxSize) {
    google::protobuf::StringValue test_message;
    test_message.set_value("test string with some content");

    const auto result = ugrpc::ToLimitedLoggingString(test_message, 0);
    EXPECT_EQ(result, kTruncateMarker);
}

UTEST(ProtobufLogging, EdgeCasesSmallMaxSizes) {
    google::protobuf::StringValue test_message;
    test_message.set_value("test string with some content");

    const auto one = ugrpc::ToLimitedLoggingString(test_message, 1);
    const auto five = ugrpc::ToLimitedLoggingString(test_message, 5);
    const auto seven = ugrpc::ToLimitedLoggingString(test_message, 7);
    const auto nine = ugrpc::ToLimitedLoggingString(test_message, 9);

    EXPECT_THAT(one, testing::EndsWith(kTruncateMarker));
    EXPECT_THAT(five, testing::EndsWith(kTruncateMarker));
    EXPECT_THAT(seven, testing::EndsWith(kTruncateMarker));
    EXPECT_THAT(nine, testing::EndsWith(kTruncateMarker));
}

UTEST(ProtobufLogging, GetErrorDetailsSizeLimiting) {
    std::string large_message(2000, 'A');
    google::rpc::Status error_details;
    error_details.set_code(static_cast<int>(grpc::StatusCode::NOT_FOUND));
    error_details.set_message("Some error");
    auto* detail = error_details.add_details();
    detail->set_type_url("type.googleapis.com/google.protobuf.StringValue");
    google::protobuf::StringValue detail_value;
    detail_value.set_value(large_message);
    detail->set_value(detail_value.SerializeAsString());
    grpc::Status
        large_error(grpc::StatusCode::NOT_FOUND, "Invalid parameter provided", error_details.SerializeAsString());
    const auto small_result = ugrpc::ToLimitedLoggingString(large_error, 100);
    EXPECT_LE(small_result.size(), 300u);

    const auto medium_result = ugrpc::ToLimitedLoggingString(large_error, 500);
    EXPECT_LE(medium_result.size(), 700u);
    EXPECT_GT(medium_result.size(), small_result.size());

    const auto large_result = ugrpc::ToLimitedLoggingString(large_error, 3000);
    EXPECT_GT(large_result.size(), medium_result.size());
}

UTEST(ProtobufLogging, GetErrorDetailsUnlimited) {
    std::string large_message(5000, 'B');
    google::rpc::Status error_details;
    auto* detail = error_details.add_details();
    detail->set_type_url("type.googleapis.com/google.protobuf.StringValue");
    google::protobuf::StringValue detail_value;
    detail_value.set_value(large_message);
    detail->set_value(detail_value.SerializeAsString());
    grpc::Status
        large_error(grpc::StatusCode::NOT_FOUND, "Invalid parameter provided", error_details.SerializeAsString());
    const auto unlimited_result = ugrpc::ToUnlimitedLoggingString(large_error);

    EXPECT_THAT(unlimited_result, testing::HasSubstr(large_message));
    EXPECT_THAT(unlimited_result, testing::HasSubstr("Invalid parameter provided"));
    EXPECT_GT(unlimited_result.size(), 5000);
}

UTEST(ProtobufLogging, EdgeCasesEmptyDetails) {
    grpc::Status status_with_empty_details(grpc::StatusCode::INTERNAL, "Internal error", "");

    const auto result = ugrpc::ToLimitedLoggingString(status_with_empty_details);
    EXPECT_THAT(result, testing::HasSubstr("INTERNAL"));
    EXPECT_THAT(result, testing::HasSubstr("Internal error"));
}

UTEST(ProtobufLogging, LogHelperOperator) {
    google::protobuf::StringValue message;
    message.set_value("stdout test message");
    LOG_INFO("Stdout test: {}", message);
}

UTEST(ProtobufLogging, LogHelperOperatorEmptyMessage) {
    google::protobuf::Empty empty_message;
    LOG_INFO("Empty protobuf: {}", empty_message);
}

UTEST(ProtobufLogging, FmtFormatterWithData) {
    std::string message = "test string with some content";
    google::protobuf::StringValue test_message;
    test_message.set_value(message);

    const auto result = fmt::format("Message: {}", test_message);
    EXPECT_THAT(result, testing::HasSubstr(message));
}

UTEST(ProtobufLogging, FmtFormatterEmptyMessage) {
    google::protobuf::Empty empty_message;
    const auto result = fmt::format("Empty: {}", empty_message);

    EXPECT_EQ(result, "Empty: {}");
}

USERVER_NAMESPACE_END
