#include <userver/engine/subprocess/child_process.hpp>

#include "child_process_impl.hpp"

USERVER_NAMESPACE_BEGIN

namespace engine::subprocess {

ChildProcess::ChildProcess(std::unique_ptr<ChildProcessImpl>&& impl)
    : impl_(std::move(impl)) {}

ChildProcess::~ChildProcess() = default;

ChildProcess::ChildProcess(ChildProcess&&) noexcept = default;

int ChildProcess::GetPid() const { return impl_->GetPid(); }

void ChildProcess::Wait() { return impl_->Wait(); }

bool ChildProcess::WaitUntil(
    const std::chrono::steady_clock::time_point& until) {
  return impl_->WaitUntil(until);
}

ChildProcessStatus ChildProcess::Get() { return impl_->Get(); }

void ChildProcess::SendSignal(int signum) { return impl_->SendSignal(signum); }

}  // namespace engine::subprocess

USERVER_NAMESPACE_END
