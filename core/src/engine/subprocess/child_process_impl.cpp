#include "child_process_impl.hpp"

#include <sys/types.h>

#include <csignal>

#include <utils/check_syscall.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::subprocess {

ChildProcessImpl::ChildProcessImpl(
    int pid, impl::BlockingFuture<ChildProcessStatus>&& status_future)
    : pid_(pid), status_future_(std::move(status_future)) {}

void ChildProcessImpl::Wait() { status_future_.wait(); }

bool ChildProcessImpl::WaitUntil(
    const std::chrono::steady_clock::time_point& until) {
  return status_future_.wait_until(until) == std::future_status::ready;
}

ChildProcessStatus ChildProcessImpl::Get() { return status_future_.get(); }

void ChildProcessImpl::SendSignal(int signum) {
  utils::CheckSyscall(kill(pid_, signum), "kill, pid={}", pid_);
}

}  // namespace engine::subprocess

USERVER_NAMESPACE_END
