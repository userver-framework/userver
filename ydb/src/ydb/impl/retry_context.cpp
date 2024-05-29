#include <ydb/impl/retry_context.hpp>

#include <ydb/impl/operation_settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

NYdb::NRetry::TRetryOperationSettings PrepareRetrySettings(
    const OperationSettings& settings, bool is_retryable) {
  NYdb::NRetry::TRetryOperationSettings result;
  if (is_retryable) {
    UASSERT(settings.retries.has_value());
    result.MaxRetries(*settings.retries);
  } else {
    result.MaxRetries(0);
  }
  if (settings.get_session_timeout_ms > std::chrono::milliseconds::zero()) {
    result.GetSessionClientTimeout(settings.get_session_timeout_ms);
  }
  return result;
}

bool IsRetryableStatus(NYdb::EStatus status) {
  return
      // retryable
      status == NYdb::EStatus::ABORTED ||
      status == NYdb::EStatus::UNAVAILABLE ||
      status == NYdb::EStatus::OVERLOADED ||
      status == NYdb::EStatus::BAD_SESSION ||
      status == NYdb::EStatus::CLIENT_RESOURCE_EXHAUSTED;
}

}  // namespace ydb::impl

USERVER_NAMESPACE_END
