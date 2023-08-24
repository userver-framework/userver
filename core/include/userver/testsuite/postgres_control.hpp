#pragma once

#include <chrono>

#include <userver/engine/deadline.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite {

class PostgresControl {
 public:
  enum class ReadonlyMaster { kNotExpected, kExpected };

  PostgresControl() = default;

  PostgresControl(std::chrono::milliseconds execute_timeout,
                  std::chrono::milliseconds statement_timeout,
                  ReadonlyMaster readonly_master);

  [[nodiscard]] engine::Deadline MakeExecuteDeadline(
      std::chrono::milliseconds duration) const;

  [[nodiscard]] std::chrono::milliseconds MakeStatementTimeout(
      std::chrono::milliseconds duration) const;

  bool IsReadonlyMasterExpected() const;

 private:
  std::chrono::milliseconds execute_timeout_{};
  std::chrono::milliseconds statement_timeout_{};
  bool is_readonly_master_expected_{false};
};

}  // namespace testsuite

USERVER_NAMESPACE_END
