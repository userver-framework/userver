#include "retry_policy.hpp"

#include <string>
#include <string_view>
#include <utility>

#include <fmt/format.h>
#include <gtest/gtest.h>

#include <userver/storages/redis/reply.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

TEST(RedisRetryPolicy, TransportStatuses) {
    struct TestCase {
        ReplyStatus status;
        RetryOutcome outcome;
    };
    const TestCase cases[] = {
        {ReplyStatus::kInputOutputError, RetryOutcome::kRetryable},
        {ReplyStatus::kEndOfFileError, RetryOutcome::kRetryable},
        {ReplyStatus::kTimeoutError, RetryOutcome::kRetryable},
        {ReplyStatus::kProtocolError, RetryOutcome::kNonRetryable},
        {ReplyStatus::kOutOfMemoryError, RetryOutcome::kNonRetryable},
        {ReplyStatus::kOtherError, RetryOutcome::kNonRetryable},
    };
    for (const auto& test : cases) {
        SCOPED_TRACE(static_cast<int>(test.status));
        const Reply reply{"command", ReplyData::CreateError("unrecognized error"), test.status};
        EXPECT_EQ(ClassifyReplyForRetry(reply), test.outcome);
    }
    const Reply success{"command", ReplyData::CreateNil()};
    EXPECT_EQ(ClassifyReplyForRetry(success), RetryOutcome::kSuccess);
}

TEST(RedisRetryPolicy, ServerErrorCodes) {
    struct TestCase {
        std::string_view code;
        RetryOutcome outcome;
    };
    const TestCase cases[] = {
        {"LOADING", RetryOutcome::kRetryable},
        {"MASTERDOWN", RetryOutcome::kRetryable},
        {"READONLY", RetryOutcome::kRetryable},
        {"CLUSTERDOWN", RetryOutcome::kRetryable},
        {"TRYAGAIN", RetryOutcome::kRetryable},
        {"MOVED", RetryOutcome::kRedirect},
        {"ASK", RetryOutcome::kRedirect},
        {"OOM", RetryOutcome::kNonRetryable},
        {"ERR", RetryOutcome::kNonRetryable},
        {"WRONGTYPE", RetryOutcome::kNonRetryable},
        {"NOAUTH", RetryOutcome::kNonRetryable},
        {"NOPERM", RetryOutcome::kNonRetryable},
        {"NOSCRIPT", RetryOutcome::kNonRetryable},
        {"UNKNOWN", RetryOutcome::kNonRetryable},
        {"TRYAGAIN_EXTRA", RetryOutcome::kNonRetryable},
        {"ERR TRYAGAIN", RetryOutcome::kNonRetryable},
        {"", RetryOutcome::kNonRetryable},
    };
    for (const auto& test : cases) {
        SCOPED_TRACE(test.code);
        for (const std::string_view suffix : {"", " explanation"}) {
            auto error = fmt::format("{}{}", test.code, suffix);
            const Reply reply{"command", ReplyData::CreateError(std::move(error)), ReplyStatus::kOtherError};
            EXPECT_EQ(ClassifyReplyForRetry(reply), test.outcome);
        }
    }
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
