#include <ydb/impl/future.hpp>

#include <ydb/impl/retry_tx.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

void HandleOnceRetry(utils::RetryBudget& retry_budget, NYdb::EStatus status) {
    auto retry_step = RetryStep::GetNext({}, status, 0);
    if (status == NYdb::EStatus::SUCCESS) {
        retry_budget.AccountOk();
    } else if (retry_step.backoff.has_value() && retry_budget.CanRetry()) {
        retry_budget.AccountFail();
    }
}

}  // namespace ydb::impl

USERVER_NAMESPACE_END
