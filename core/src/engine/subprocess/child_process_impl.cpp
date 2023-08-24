#include "child_process_impl.hpp"

#include <sys/types.h>

#include <csignal>

#include <userver/engine/task/cancel.hpp>
#include <utils/check_syscall.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::subprocess {

ChildProcessImpl::ChildProcessImpl(int pid,
                                   Future<ChildProcessStatus>&& status_future)
    : pid_(pid), status_future_(std::move(status_future)) {}

void ChildProcessImpl::WaitNonCancellable() {
  TaskCancellationBlocker cancel_blocker;
  const auto status = status_future_.wait();
  UASSERT(status == FutureStatus::kReady);
}

bool ChildProcessImpl::WaitUntil(engine::Deadline deadline) {
  return status_future_.wait_until(deadline) == engine::FutureStatus::kReady;
}

ChildProcessStatus ChildProcessImpl::Get() {
  TaskCancellationBlocker cancel_blocker;
  return status_future_.get();
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void ChildProcessImpl::SendSignal(int signum) {
  utils::CheckSyscall(kill(pid_, signum), "kill, pid={}", pid_);
}

}  // namespace engine::subprocess

USERVER_NAMESPACE_END
