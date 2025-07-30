#include <cstddef>
#include <vector>

#include <gmock/gmock.h>
#include <grpcpp/support/status.h>
#include <gtest/gtest.h>

#include <userver/engine/task/task.hpp>
#include <userver/engine/wait_all_checked.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>

#include <userver/grpc-protovalidate/validate.hpp>
#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

UTEST(ValidateTest, Valid) {
    auto result = grpc_protovalidate::ValidateMessage(tests::CreateValidMessage(2));
    EXPECT_TRUE(result.IsSuccess());
    UEXPECT_THROW_MSG(result.GetError(), std::logic_error, "Requested error for success validation result");
}

UTEST_MT(ValidateTest, ValidMultithreaded, 4) {
    std::vector<engine::Task> tasks;
    for (std::size_t i = 1; i < 250; ++i) {
        tasks.emplace_back(utils::Async("ValidateTestMultithreaded", [i]() {
            auto _ = grpc_protovalidate::ValidateMessage(tests::CreateValidMessage(i));
        }));
    }
    UEXPECT_NO_THROW(engine::WaitAllChecked(tasks));
}

UTEST(EnsureValidTest, InvalidDefault) {
    constexpr auto kExpectedMessage = "Message 'types.ConstrainedMessage' validation error: 18 constraint(s) violated";
    auto result = grpc_protovalidate::ValidateMessage(tests::CreateInvalidMessage());

    ASSERT_FALSE(result.IsSuccess());
    const auto error = std::move(result).GetError();

    EXPECT_EQ(error.GetType(), grpc_protovalidate::ValidationError::Type::kRule);
    EXPECT_EQ(error.GetMessageName(), "types.ConstrainedMessage");
    EXPECT_EQ(error.GetDescription(), kExpectedMessage);
    EXPECT_THAT(error.GetViolations(), testing::SizeIs(18));

    const grpc::Status status_no_details = error.GetGrpcStatus(false);
    EXPECT_EQ(status_no_details.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
    EXPECT_EQ(status_no_details.error_message(), kExpectedMessage);

    const grpc::Status status_with_details = error.GetGrpcStatus(true);
    EXPECT_EQ(status_with_details.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
    EXPECT_EQ(status_with_details.error_message(), kExpectedMessage);
    EXPECT_GT(status_with_details.error_details().size(), 0);
}

UTEST(EnsureValidTest, InvalidFailFast) {
    constexpr auto kExpectedMessage = "Message 'types.ConstrainedMessage' validation error: 1 constraint(s) violated";
    const auto result = grpc_protovalidate::ValidateMessage(tests::CreateInvalidMessage(), {.fail_fast = true});

    ASSERT_FALSE(result.IsSuccess());
    const auto error = std::move(result).GetError();

    EXPECT_EQ(error.GetType(), grpc_protovalidate::ValidationError::Type::kRule);
    EXPECT_EQ(error.GetMessageName(), "types.ConstrainedMessage");
    EXPECT_EQ(error.GetDescription(), kExpectedMessage);
    EXPECT_THAT(error.GetViolations(), testing::SizeIs(1));

    const grpc::Status status_no_details = error.GetGrpcStatus(false);
    EXPECT_EQ(status_no_details.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
    EXPECT_EQ(status_no_details.error_message(), kExpectedMessage);

    const grpc::Status status_with_details = error.GetGrpcStatus(true);
    EXPECT_EQ(status_with_details.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
    EXPECT_EQ(status_with_details.error_message(), kExpectedMessage);
    EXPECT_GT(status_with_details.error_details().size(), 0);
}

UTEST(EnsureValidTest, InvalidConstraintsDefault) {
    constexpr auto kExpectedMessage =
        "Message 'types.InvalidConstraints' validation error: internal protovalidate error (check constraints "
        "syntax in the proto file) - INVALID_ARGUMENT: no_such_field : non_existent_field";
    const auto result = grpc_protovalidate::ValidateMessage(types::InvalidConstraints(), {.fail_fast = true});

    ASSERT_FALSE(result.IsSuccess());
    const auto error = std::move(result).GetError();

    EXPECT_EQ(error.GetType(), grpc_protovalidate::ValidationError::Type::kInternal);
    EXPECT_EQ(error.GetMessageName(), "types.InvalidConstraints");
    EXPECT_EQ(error.GetDescription(), kExpectedMessage);
    EXPECT_THAT(error.GetViolations(), testing::SizeIs(0));

    const grpc::Status status = error.GetGrpcStatus();
    EXPECT_EQ(status.error_code(), grpc::StatusCode::INTERNAL);
    EXPECT_EQ(status.error_message(), kExpectedMessage);
}

USERVER_NAMESPACE_END
