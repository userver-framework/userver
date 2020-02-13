#pragma once

#include <chrono>

#include <engine/deadline.hpp>

namespace testsuite {

class PostgresControl {
 public:
  PostgresControl(std::chrono::milliseconds execute_timeout,
                  std::chrono::milliseconds statement_timeout);

  PostgresControl();

  [[nodiscard]] engine::Deadline MakeExecuteDeadline(
      std::chrono::milliseconds duration) const;

  [[nodiscard]] std::chrono::milliseconds MakeStatementTimeout(
      std::chrono::milliseconds duration) const;

 private:
  std::chrono::milliseconds execute_timeout_;
  std::chrono::milliseconds statement_timeout_;
};

}  // namespace testsuite
