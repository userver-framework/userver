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

    const auto result = ugrpc::ToLimitedDebugString(test_message, 1000);

    EXPECT_THAT(result, testing::HasSubstr("test string with some content"));
    EXPECT_THAT(result, testing::HasSubstr("value:"));
}

UTEST(ProtobufLogging, GetMessageEmptyMessage) {
    google::protobuf::Empty empty_message;
    const auto result = ugrpc::ToLimitedDebugString(empty_message, 100);

    EXPECT_EQ(result, "<EMPTY>");
}

UTEST(ProtobufLogging, GetMessageSizeLimit) {
    google::protobuf::StringValue test_message;
    test_message.set_value("test string with some content");

    const auto small_result = ugrpc::ToLimitedDebugString(test_message, 10);
    const auto large_result = ugrpc::ToLimitedDebugString(test_message, 1000);

    EXPECT_LE(small_result.size(), kTruncateMarker.size());
    EXPECT_GT(large_result.size(), small_result.size());
    EXPECT_THAT(large_result, testing::HasSubstr("test string with some content"));
}

UTEST(ProtobufLogging, OkStatus) {
    grpc::Status ok_status = grpc::Status::OK;
    const auto result = ugrpc::ToUnlimitedDebugString(ok_status);

    EXPECT_EQ(result, "OK");
}

UTEST(ProtobufLogging, SimpleError) {
    grpc::Status simple_error(grpc::StatusCode::NOT_FOUND, "Resource not found");
    const auto result = ugrpc::ToUnlimitedDebugString(simple_error);

    EXPECT_THAT(result, testing::HasSubstr("NOT_FOUND"));
    EXPECT_THAT(result, testing::HasSubstr("Resource not found"));
    EXPECT_THAT(result, testing::HasSubstr("code:"));
    EXPECT_THAT(result, testing::HasSubstr("error message:"));
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

    const auto result = ugrpc::ToUnlimitedDebugString(complex_status);

    EXPECT_THAT(result, testing::HasSubstr("INVALID_ARGUMENT"));
    EXPECT_THAT(result, testing::HasSubstr("Invalid parameter provided"));
    EXPECT_THAT(result, testing::HasSubstr("error details:"));
    EXPECT_THAT(result, testing::HasSubstr("Additional error context"));
    EXPECT_THAT(result, testing::HasSubstr("type.googleapis.com"));
}

UTEST(ProtobufLogging, EdgeCasesZeroMaxSize) {
    google::protobuf::StringValue test_message;
    test_message.set_value("test string with some content");

    const auto result = ugrpc::ToLimitedDebugString(test_message, 0);
    EXPECT_EQ(result, kTruncateMarker);
}

UTEST(ProtobufLogging, EdgeCasesSmallMaxSizes) {
    google::protobuf::StringValue test_message;
    test_message.set_value("test string with some content");

    const auto one = ugrpc::ToLimitedDebugString(test_message, 1);
    const auto five = ugrpc::ToLimitedDebugString(test_message, 5);
    const auto seven = ugrpc::ToLimitedDebugString(test_message, 7);  // Length of <EMPTY>
    const auto nine = ugrpc::ToLimitedDebugString(test_message, 9);

    EXPECT_EQ(one, kTruncateMarker);
    EXPECT_EQ(five, kTruncateMarker);
    EXPECT_EQ(seven, kTruncateMarker);
    EXPECT_EQ(nine, kTruncateMarker);
}

UTEST(ProtobufLogging, GetErrorDetailsSizeLimiting) {
    std::string large_message(2000, 'A');
    google::rpc::Status error_details;
    error_details.set_code(static_cast<int>(grpc::StatusCode::NOT_FOUND));
    error_details.set_message("Some error");
    auto* detail = error_details.add_details();
    google::protobuf::StringValue detail_value;
    detail_value.set_value(large_message);
    detail->set_value(detail_value.SerializeAsString());
    grpc::Status
        large_error(grpc::StatusCode::NOT_FOUND, "Invalid parameter provided", error_details.SerializeAsString());
    const auto small_result = ugrpc::ToLimitedDebugString(large_error, 100);
    EXPECT_LE(small_result.size(), 200u);

    const auto medium_result = ugrpc::ToLimitedDebugString(large_error, 500);
    EXPECT_LE(medium_result.size(), 600u);
    EXPECT_GT(medium_result.size(), small_result.size());

    const auto large_result = ugrpc::ToLimitedDebugString(large_error, 3000);
    EXPECT_GT(large_result.size(), medium_result.size());
}

UTEST(ProtobufLogging, GetErrorDetailsUnlimited) {
    std::string large_message(5000, 'B');
    google::rpc::Status error_details;
    auto* detail = error_details.add_details();
    google::protobuf::StringValue detail_value;
    detail_value.set_value(large_message);
    detail->set_value(detail_value.SerializeAsString());
    grpc::Status
        large_error(grpc::StatusCode::NOT_FOUND, "Invalid parameter provided", error_details.SerializeAsString());
    const auto unlimited_result = ugrpc::ToUnlimitedDebugString(large_error);

    EXPECT_THAT(unlimited_result, testing::HasSubstr(large_message));
    EXPECT_THAT(unlimited_result, testing::HasSubstr("Invalid parameter provided"));
    EXPECT_GT(unlimited_result.size(), 5000);
}

UTEST(ProtobufLogging, EdgeCasesEmptyDetails) {
    grpc::Status status_with_empty_details(grpc::StatusCode::INTERNAL, "Internal error", "");

    const auto result = ugrpc::ToLimitedDebugString(status_with_empty_details);
    EXPECT_THAT(result, testing::HasSubstr("INTERNAL"));
    EXPECT_THAT(result, testing::HasSubstr("Internal error"));
    EXPECT_THAT(result, testing::Not(testing::HasSubstr("error details:")));
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

    EXPECT_EQ(result, "Empty: <EMPTY>");
}

USERVER_NAMESPACE_END
