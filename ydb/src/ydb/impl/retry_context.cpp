#include <ydb/impl/retry_context.hpp>

#include <userver/engine/deadline.hpp>
#include <userver/utils/retry_budget.hpp>

#include <ydb/impl/driver.hpp>
#include <ydb/impl/operation_settings.hpp>
#include <ydb/impl/request_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

namespace {

NYdb::NRetry::TRetryOperationSettings PrepareRetrySettings(
    const OperationSettings& settings, bool is_retryable) {
  NYdb::NRetry::TRetryOperationSettings result;
  if (settings.retries > 0 && is_retryable) {
    result.MaxRetries(settings.retries);
  } else {
    result.MaxRetries(0);
  }
  if (settings.get_session_timeout_ms > std::chrono::milliseconds::zero()) {
    result.GetSessionClientTimeout(settings.get_session_timeout_ms);
  }
  return result;
}

}  // namespace

bool IsRetryableStatus(NYdb::EStatus status) {
  return
      // retryable
      status == NYdb::EStatus::ABORTED ||
      status == NYdb::EStatus::UNAVAILABLE ||
      status == NYdb::EStatus::OVERLOADED ||
      status == NYdb::EStatus::BAD_SESSION ||
      status == NYdb::EStatus::CLIENT_RESOURCE_EXHAUSTED;
}

void HandleOnceRetry(utils::RetryBudget& retry_budget, NYdb::EStatus status) {
  if (IsRetryableStatus(status) && retry_budget.CanRetry()) {
    retry_budget.AccountFail();
  } else if (status == NYdb::EStatus::SUCCESS) {
    retry_budget.AccountOk();
  }
}

RetryContext::RetryContext(TableClient& table_client,
                           OperationSettings& settings)
    : retry_budget(table_client.driver_->GetRetryBudget()),
      retry_settings(
          impl::PrepareRetrySettings(settings, retry_budget.CanRetry())) {}

bool RetryContext::ApplyStatusAndCanRetry(NYdb::EStatus status) {
  if (IsRetryableStatus(status) && retry_budget.CanRetry()) {
    retry_budget.AccountFail();
    retries++;
    return true;
  }
  if (status == NYdb::EStatus::SUCCESS) {
    retry_budget.AccountOk();
    // iff false => caller try to retry on success status, so return true
    return true;
  }
  return false;
}

}  // namespace ydb::impl

USERVER_NAMESPACE_END
