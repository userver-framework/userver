#include <userver/testsuite/postgres_control.hpp>

#include <algorithm>
#include <stdexcept>

USERVER_NAMESPACE_BEGIN

namespace testsuite {

PostgresControl::PostgresControl(
    std::chrono::milliseconds network_timeout,
    std::chrono::milliseconds statement_timeout,
    ReadonlyMaster readonly_master
)
    : network_timeout_(network_timeout),
      statement_timeout_(statement_timeout),
      is_readonly_master_expected_(readonly_master == ReadonlyMaster::kExpected) {
    if (network_timeout.count() < 0) {
        throw std::invalid_argument("Negative network_timeout");
    }
    if (statement_timeout.count() < 0) {
        throw std::invalid_argument("Negative statement_timeout");
    }
}

[[nodiscard]] engine::Deadline PostgresControl::MakeExecuteDeadline(std::chrono::milliseconds duration) const {
    if (network_timeout_ != std::chrono::milliseconds::zero()) {
        return engine::Deadline::FromDuration(std::max(duration, network_timeout_));
    }
    return engine::Deadline::FromDuration(duration);
}

[[nodiscard]] std::chrono::milliseconds PostgresControl::MakeStatementTimeout(std::chrono::milliseconds duration
) const {
    if (statement_timeout_ != std::chrono::milliseconds::zero()) {
        return std::max(duration, statement_timeout_);
    }
    return duration;
}

bool PostgresControl::IsReadonlyMasterExpected() const { return is_readonly_master_expected_; }

}  // namespace testsuite

USERVER_NAMESPACE_END
