#include <ydb/impl/retry.hpp>

#include <ydb-cpp-sdk/library/issue/yql_issue.h>

#include <ydb/impl/operation_settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

NYdb::NRetry::TRetryOperationSettings PrepareRetrySettings(
    const OperationSettings& operation_settings,
    const utils::RetryBudget& retry_budget,
    engine::Deadline deadline
) {
    NYdb::NRetry::TRetryOperationSettings retry_settings;

    UASSERT(operation_settings.retries.has_value());
    retry_settings.MaxRetries(retry_budget.CanRetry() ? operation_settings.retries.value() : 0);

    retry_settings.GetSessionClientTimeout(GetBoundTimeout(operation_settings.get_session_timeout_ms, deadline));

    return retry_settings;
}

bool IsRetryableStatus(NYdb::EStatus status) {
    switch (status) {
        case NYdb::EStatus::ABORTED:
        case NYdb::EStatus::UNAVAILABLE:
        case NYdb::EStatus::OVERLOADED:
        case NYdb::EStatus::BAD_SESSION:
        case NYdb::EStatus::CLIENT_RESOURCE_EXHAUSTED:
            return true;

        default:
            return false;
    }
}

NYdb::TStatus MakeNonRetryableStatus() { return NYdb::TStatus{NYdb::EStatus::BAD_REQUEST, NYdb::NIssue::TIssues{}}; }

}  // namespace ydb::impl

USERVER_NAMESPACE_END
