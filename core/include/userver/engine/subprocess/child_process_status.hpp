#pragma once

/// @file userver/engine/subprocess/child_process_status.hpp
/// @brief @copybrief engine::subprocess::ChildProcessStatus

#include <chrono>
#include <string>

#include <userver/utils/traceful_exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::subprocess {

/// status of finished subprocess
class ChildProcessStatus {
 public:
  ChildProcessStatus(int status, std::chrono::milliseconds execution_time);

  enum class ExitReason { kExited, kSignaled };

  ExitReason GetExitReason() const { return exit_reason_; }

  bool IsExited() const { return exit_reason_ == ExitReason::kExited; }

  /// can be called if `IsExited()` returns true
  int GetExitCode() const;

  bool IsSignaled() const { return exit_reason_ == ExitReason::kSignaled; }

  /// can be called if `IsSignaled()` returns true
  int GetTermSignal() const;

  /// Returns execution time for subprocess + time to start it with `execve()`
  std::chrono::milliseconds GetExecutionTime() const { return execution_time_; }

 private:
  const std::chrono::milliseconds execution_time_;
  const ExitReason exit_reason_;
  const int exit_code_;
  const int term_signal_;
};

class ChildProcessStatusException : public utils::TracefulException {
 public:
  using utils::TracefulException::TracefulException;
};

const std::string& ToString(ChildProcessStatus::ExitReason exit_reason);

}  // namespace engine::subprocess

USERVER_NAMESPACE_END
