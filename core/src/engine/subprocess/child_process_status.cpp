#include <userver/engine/subprocess/child_process_status.hpp>

#include <sys/types.h>
#include <sys/wait.h>

USERVER_NAMESPACE_BEGIN

namespace engine::subprocess {
namespace {

ChildProcessStatus::ExitReason ExitReasonFromStatus(int status) {
  if (WIFEXITED(status)) return ChildProcessStatus::ExitReason::kExited;
  if (WIFSIGNALED(status)) return ChildProcessStatus::ExitReason::kSignaled;
  throw ChildProcessStatusException("can't create ExitReason from status=" +
                                    std::to_string(status));
}

}  // namespace

ChildProcessStatus::ChildProcessStatus(int status,
                                       std::chrono::milliseconds execution_time)
    : execution_time_(execution_time),
      exit_reason_(ExitReasonFromStatus(status)),
      exit_code_(IsExited() ? WEXITSTATUS(status) : -1),
      term_signal_(IsSignaled() ? WTERMSIG(status) : -1) {}

int ChildProcessStatus::GetExitCode() const {
  if (!IsExited())
    throw ChildProcessStatusException("Child process did not exit normally.");
  return exit_code_;
}

int ChildProcessStatus::GetTermSignal() const {
  if (!IsSignaled())
    throw ChildProcessStatusException(
        "Child process was not terminated by a signal.");
  return term_signal_;
}

const std::string& ToString(ChildProcessStatus::ExitReason exit_reason) {
  static const std::string kExited = "exited";
  static const std::string kSignaled = "signaled";
  switch (exit_reason) {
    case ChildProcessStatus::ExitReason::kExited:
      return kExited;
    case ChildProcessStatus::ExitReason::kSignaled:
      return kSignaled;
  }
  throw ChildProcessStatusException(
      "unknown ExitReason: " + std::to_string(static_cast<int>(exit_reason)));
}

}  // namespace engine::subprocess

USERVER_NAMESPACE_END
