#include "retry_policy.hpp"

#include <string_view>

#include <userver/storages/redis/reply.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

namespace {

RetryOutcome ClassifyServerError(std::string_view error) {
    const auto code = error.substr(0, error.find(' '));
    if (code == "MOVED" || code == "ASK") {
        return RetryOutcome::kRedirect;
    }
    if (code == "LOADING" || code == "MASTERDOWN" || code == "READONLY" || code == "CLUSTERDOWN" || code == "TRYAGAIN")
    {
        return RetryOutcome::kRetryable;
    }
    return RetryOutcome::kNonRetryable;
}

}  // namespace

RetryOutcome ClassifyReplyForRetry(const Reply& reply) {
    switch (reply.status) {
        case ReplyStatus::kOk:
            return RetryOutcome::kSuccess;
        case ReplyStatus::kInputOutputError:
        case ReplyStatus::kEndOfFileError:
        case ReplyStatus::kTimeoutError:
            return RetryOutcome::kRetryable;
        case ReplyStatus::kProtocolError:
        case ReplyStatus::kOutOfMemoryError:
            return RetryOutcome::kNonRetryable;
        case ReplyStatus::kOtherError:
            return reply.data.IsError() ? ClassifyServerError(reply.data.GetError()) : RetryOutcome::kNonRetryable;
    }
    UINVARIANT(false, "Unexpected Redis reply status");
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
