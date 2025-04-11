#include <ydb/impl/future.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

void HandleOnceRetry(utils::RetryBudget& retry_budget, NYdb::EStatus status) {
    if (IsRetryableStatus(status) && retry_budget.CanRetry()) {
        retry_budget.AccountFail();
    } else if (status == NYdb::EStatus::SUCCESS) {
        retry_budget.AccountOk();
    }
}

}  // namespace ydb::impl

USERVER_NAMESPACE_END
