#include <userver/engine/impl/blocking_future_state.hpp>

#include <engine/impl/wait_list_light.hpp>
#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

BlockingFutureStateBase::BlockingFutureStateBase() noexcept
    : is_ready_{false}, is_retrieved_{false}, finish_waiters_{} {}

bool BlockingFutureStateBase::IsReady() const noexcept {
  return is_ready_.load(std::memory_order_relaxed);
}

void BlockingFutureStateBase::Wait() {
  engine::TaskCancellationBlocker block_cancel;
  std::unique_lock<std::mutex> lock(mutex_);
  [[maybe_unused]] bool is_ready =
      result_cv_.WaitUntil(lock, {}, [this] { return IsReady(); });
  UASSERT(is_ready);
}

std::future_status BlockingFutureStateBase::WaitUntil(Deadline deadline) {
  std::unique_lock<std::mutex> lock(mutex_);
  return result_cv_.WaitUntil(lock, deadline, [this] { return IsReady(); })
             ? std::future_status::ready
             : std::future_status::timeout;
}

void BlockingFutureStateBase::EnsureNotRetrieved() {
  if (is_retrieved_.exchange(true)) {
    throw std::future_error(std::future_errc::future_already_retrieved);
  }
}

BlockingFutureStateBase::Lock::Lock(BlockingFutureStateBase& self)
    : self_(self) {
  std::unique_lock lock(self_.mutex_);
  if (self_.is_ready_.exchange(true, std::memory_order_relaxed)) {
    throw std::future_error(std::future_errc::promise_already_satisfied);
  }
  lock.release();  // not unlock!
}

BlockingFutureStateBase::Lock::~Lock() {
  self_.mutex_.unlock();

  self_.finish_waiters_->WakeupOne();
  self_.result_cv_.NotifyAll();
}

BlockingFutureStateBase::~BlockingFutureStateBase() = default;

void BlockingFutureStateBase::AppendWaiter(
    impl::TaskContext& context) noexcept {
  finish_waiters_->Append(&context);
}

void BlockingFutureStateBase::RemoveWaiter(
    impl::TaskContext& context) noexcept {
  finish_waiters_->Remove(context);
}

void BlockingFutureStateBase::WakeupAllWaiters() {
  finish_waiters_->WakeupOne();
}

bool BlockingFutureStateBase::IsWaitingEnabledFrom(
    const impl::TaskContext& /*context*/) const noexcept {
  return true;
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
