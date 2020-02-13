#include <testsuite/postgres_control.hpp>

namespace testsuite {

PostgresControl::PostgresControl(std::chrono::milliseconds execute_timeout,
                                 std::chrono::milliseconds statement_timeout)
    : execute_timeout_(execute_timeout), statement_timeout_(statement_timeout) {
  if (execute_timeout.count() < 0) {
    throw std::invalid_argument("Negative execute_timeout");
  }
  if (statement_timeout.count() < 0) {
    throw std::invalid_argument("Negative statement_timeout");
  }
}

PostgresControl::PostgresControl() : execute_timeout_(), statement_timeout_() {}

[[nodiscard]] engine::Deadline PostgresControl::MakeExecuteDeadline(
    std::chrono::milliseconds duration) const {
  if (execute_timeout_ != std::chrono::milliseconds::zero()) {
    return engine::Deadline::FromDuration(std::max(duration, execute_timeout_));
  }
  return engine::Deadline::FromDuration(duration);
}

[[nodiscard]] std::chrono::milliseconds PostgresControl::MakeStatementTimeout(
    std::chrono::milliseconds duration) const {
  if (statement_timeout_ != std::chrono::milliseconds::zero()) {
    return std::max(duration, statement_timeout_);
  }
  return duration;
}

}  // namespace testsuite
