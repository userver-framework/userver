#pragma once

/// @file userver/engine/subprocess/child_process.hpp
/// @brief @copybrief engine::subprocess::ChildProcess

#include <chrono>
#include <memory>

#include <userver/engine/subprocess/child_process_status.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {
namespace subprocess {

class ChildProcessImpl;

class ChildProcess final {
 public:
  explicit ChildProcess(std::unique_ptr<ChildProcessImpl>&& impl);
  /// It destroys `ChildProcess` object but it does not terminate the child
  /// process (just detach from the child process).
  ~ChildProcess();

  ChildProcess(ChildProcess&&) noexcept;

  /// Returns pid of the child process.
  int GetPid() const;

  /// Wait for the child process to terminate.
  void Wait();

  /// Wait for the child process to terminate.
  /// Returns if this did not happen for the specified timeout duration.
  template <typename Rep, typename Period>
  bool WaitFor(const std::chrono::duration<Rep, Period>& duration) {
    return WaitUntil(std::chrono::steady_clock::now() + duration);
  }

  /// Wait for the child process to terminate.
  /// Returns if this did not happen until specified time point has been
  /// reached.
  template <typename Clock, typename Duration>
  bool WaitUntil(const std::chrono::time_point<Clock, Duration>& until) {
    return WaitFor(until - Clock::now());
  }

  /// Wait for the child process to terminate.
  /// Returns if this did not happen until specified time point has been
  /// reached.
  bool WaitUntil(const std::chrono::steady_clock::time_point& until);

  /// Wait for the child process to terminate.
  /// Returns `ChildProcessStatus` of finished subprocess.
  ChildProcessStatus Get();

  /// Send a signal to the child process.
  void SendSignal(int signum);

 private:
  std::unique_ptr<ChildProcessImpl> impl_;
};

}  // namespace subprocess
}  // namespace engine

USERVER_NAMESPACE_END
