#include <ydb/impl/retry_tx.hpp>

#include <ydb-cpp-sdk/library/issue/yql_issue.h>

#include <userver/utils/rand.hpp>
#include <ydb/impl/operation_settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

namespace {

constexpr std::chrono::milliseconds kMaxBackoff = std::chrono::hours(1);

std::chrono::milliseconds CalcBackoffTime(const BackoffSettings& settings, std::uint32_t retry_number) {
    using BackoffDuration = std::chrono::duration<double, std::micro>;

    std::uint32_t backoff_slots = 1 << std::min(retry_number, settings.ceiling);

    double uncertainty_ratio = std::max(std::min(settings.uncertain_ratio, 1.0), 0.0);
    double uncertainty_multiplier = (utils::RandRange<double>(0.0, 1.0) * uncertainty_ratio) - uncertainty_ratio + 1.0;

    auto backoff = BackoffDuration(settings.slot_duration_ms) * backoff_slots * uncertainty_multiplier;
    auto backoff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(backoff);

    return std::max(std::min(backoff_ms, kMaxBackoff), std::chrono::milliseconds::zero());
}

}  // namespace

CommonRetrySettings PrepareRetrySettings(
    const RetryTxSettings& retry_tx_settings,
    const utils::RetryBudget& retry_budget,
    engine::Deadline deadline
) {
    return CommonRetrySettings{
        .timeout_ms = GetBoundTimeout(retry_tx_settings.timeout_ms, deadline),
        .retries = retry_budget.CanRetry() ? retry_tx_settings.retries.value() : 0,
        .is_idempotent = retry_tx_settings.is_idempotent,
    };
}

RetryStep RetryStep::GetNext(
    const CommonRetrySettings& retry_settings,
    NYdb::EStatus status,
    std::uint32_t retry_number
) {
    switch (status) {
        case NYdb::EStatus::ABORTED:
            return {.backoff = std::chrono::milliseconds::zero(), .reset_session = false};

        case NYdb::EStatus::OVERLOADED:
        case NYdb::EStatus::CLIENT_RESOURCE_EXHAUSTED:
            return {
                .backoff = CalcBackoffTime(retry_settings.slow_backoff_settings, retry_number),
                .reset_session = false
            };

        case NYdb::EStatus::UNAVAILABLE:
            return {
                .backoff = CalcBackoffTime(retry_settings.fast_backoff_settings, retry_number),
                .reset_session = false
            };

        case NYdb::EStatus::BAD_SESSION:
        case NYdb::EStatus::SESSION_BUSY:
            return {.backoff = std::chrono::milliseconds::zero(), .reset_session = true};

        case NYdb::EStatus::UNDETERMINED:
            if (retry_settings.is_idempotent) {
                return {
                    .backoff = CalcBackoffTime(retry_settings.fast_backoff_settings, retry_number),
                    .reset_session = false
                };
            } else {
                return {.backoff = std::nullopt, .reset_session = false};
            }

        case NYdb::EStatus::TRANSPORT_UNAVAILABLE:
            if (retry_settings.is_idempotent) {
                return {
                    .backoff = CalcBackoffTime(retry_settings.fast_backoff_settings, retry_number),
                    .reset_session = true
                };
            } else {
                return {.backoff = std::nullopt, .reset_session = false};
            }

        default:
            return {.backoff = std::nullopt, .reset_session = false};
    }
}

}  // namespace ydb::impl

USERVER_NAMESPACE_END
