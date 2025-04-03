#include <ugrpc/client/impl/retry_policy.hpp>

#include <algorithm>
#include <cmath>

#include <google/protobuf/util/time_util.h>

#include <grpc/grpc.h>  // for GRPC_ARG_EXPERIMENTAL_ENABLE_HEDGING

#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/value_builder.hpp>

#include <userver/ugrpc/impl/to_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

namespace {

constexpr std::uint32_t kMaxAttempts = 5;

constexpr std::int64_t kInitialBackoffMs = 10;
constexpr std::int64_t kMaxBackoffMs = 300;

constexpr float kBackoffMultiplier = 2.0;

}  // namespace

formats::json::Value ConstructDefaultRetryPolicy() {
    formats::json::ValueBuilder retry_policy;
    retry_policy["maxAttempts"] = kMaxAttempts;
    retry_policy["initialBackoff"] = ugrpc::impl::ToString(google::protobuf::util::TimeUtil::ToString(
        google::protobuf::util::TimeUtil::MillisecondsToDuration(kInitialBackoffMs)
    ));
    retry_policy["maxBackoff"] = ugrpc::impl::ToString(google::protobuf::util::TimeUtil::ToString(
        google::protobuf::util::TimeUtil::MillisecondsToDuration(kMaxBackoffMs)
    ));
    retry_policy["backoffMultiplier"] = kBackoffMultiplier;
    retry_policy["retryableStatusCodes"] = formats::json::MakeArray("UNAVAILABLE");
    return retry_policy.ExtractValue();
}

std::chrono::milliseconds CalculateTotalTimeout(std::chrono::milliseconds timeout, std::uint32_t attempts) {
    // Values greater than 5 are treated as 5 without being considered a validation error.
    attempts = std::min(attempts, kMaxAttempts);

    const std::int64_t timeout_ms = timeout.count();

    std::int64_t total_timeout_ms = timeout_ms;
#ifdef GRPC_ARG_EXPERIMENTAL_ENABLE_HEDGING
    for (std::uint32_t n = 1; n < attempts; ++n) {
        // https://github.com/grpc/proposal/blob/master/A6-client-retries.md
        // The initial retry attempt will occur after initialBackoff * random(0.8, 1.2).
        // After that, the n-th attempt will occur after min(initialBackoff*backoffMultiplier**(n-1), maxBackoff) *
        // random(0.8, 1.2)).
        const double backoff = std::min(
            kInitialBackoffMs * std::pow(static_cast<double>(kBackoffMultiplier), n - 1),
            static_cast<double>(kMaxBackoffMs)
        );
        // Jitter of plus or minus 0.2 is applied to the backoff delay
        constexpr double kBackoffJitter = 0.2;
        total_timeout_ms += timeout_ms + static_cast<std::int64_t>(std::ceil(backoff * (1 + kBackoffJitter)));
    }
#endif  // GRPC_ARG_EXPERIMENTAL_ENABLE_HEDGING
    return std::chrono::milliseconds{total_timeout_ms};
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
