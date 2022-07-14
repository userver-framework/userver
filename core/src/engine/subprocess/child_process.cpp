#include <userver/engine/subprocess/child_process.hpp>

#include "child_process_impl.hpp"

USERVER_NAMESPACE_BEGIN

namespace engine::subprocess {

ChildProcess::ChildProcess(ChildProcessImpl&& impl) noexcept
    : impl_(std::move(impl)) {}

ChildProcess::ChildProcess(ChildProcess&&) noexcept = default;

ChildProcess& ChildProcess::operator=(ChildProcess&&) noexcept = default;

ChildProcess::~ChildProcess() = default;

int ChildProcess::GetPid() const { return impl_->GetPid(); }

void ChildProcess::Wait() { impl_->WaitNonCancellable(); }

bool ChildProcess::WaitUntil(Deadline deadline) {
  return impl_->WaitUntil(deadline);
}

ChildProcessStatus ChildProcess::Get() { return impl_->Get(); }

void ChildProcess::SendSignal(int signum) { return impl_->SendSignal(signum); }

}  // namespace engine::subprocess

USERVER_NAMESPACE_END
