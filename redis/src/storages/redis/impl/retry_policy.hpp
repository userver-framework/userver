#pragma once

#include <userver/storages/redis/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

enum class RetryOutcome { kSuccess, kRetryable, kNonRetryable, kRedirect };

RetryOutcome ClassifyReplyForRetry(const Reply& reply);

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
