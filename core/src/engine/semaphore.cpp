#include <engine/semaphore.hpp>

#include <engine/task/cancel.hpp>
#include "task/task_context.hpp"

namespace engine {

namespace impl {
namespace {
class SemaphoreWaitPolicy final : public WaitStrategy {
 public:
  SemaphoreWaitPolicy(const std::shared_ptr<impl::WaitList>& waiters,
                      TaskContext* current, Deadline deadline)
      : WaitStrategy(deadline),
        waiters_(waiters),
        current_(current),
        lock_{*waiters_} {
    UASSERT(current_);
  }

  void AfterAsleep() override {
    waiters_->Append(lock_, current_);
    lock_.Release();
  }

  void BeforeAwake() override { lock_.Acquire(); }

  std::shared_ptr<WaitListBase> GetWaitList() override { return waiters_; }

 private:
  const std::shared_ptr<impl::WaitList>& waiters_;
  TaskContext* const current_;
  WaitList::Lock lock_;
};
}  // namespace
}  // namespace impl

Semaphore::Semaphore(std::size_t max_simultaneous_locks)
    : lock_waiters_(std::make_shared<impl::WaitList>()),
      remaining_simultaneous_locks_(max_simultaneous_locks) {
  UASSERT(max_simultaneous_locks > 0);
}

bool Semaphore::LockFastPath() {
  LOG_TRACE() << "trying fast path";
  auto expected = remaining_simultaneous_locks_.load(std::memory_order_acquire);
  if (expected == 0) expected = 1;

  if (remaining_simultaneous_locks_.compare_exchange_strong(
          expected, expected - 1, std::memory_order_relaxed)) {
    LOG_TRACE() << "fast path succeeded";
    return true;
  }
  return false;
}

bool Semaphore::LockSlowPath(Deadline deadline) {
  LOG_TRACE() << "trying slow path";

  auto* current = current_task::GetCurrentTaskContext();
  impl::SemaphoreWaitPolicy wait_manager(lock_waiters_, current, deadline);

  auto expected = remaining_simultaneous_locks_.load(std::memory_order_relaxed);
  if (expected == 0) expected = 1;

  while (!remaining_simultaneous_locks_.compare_exchange_strong(
      expected, expected - 1, std::memory_order_relaxed)) {
    LOG_TRACE() << "iteration()";
    if (expected > 0) {
      // We may acquire the lock, but the `expected` before CAS had an
      // outdated value.
      continue;
    }

    // `expected` is 0, expecting 1 to appear soon.
    expected = 1;

    LOG_TRACE() << "iteration() sleep";
    current->Sleep(&wait_manager);
    if (current->GetWakeupSource() ==
        impl::TaskContext::WakeupSource::kDeadlineTimer) {
      LOG_TRACE() << "deadline reached";
      return false;
    }
  }
  LOG_TRACE() << "slow path succeeded";
  return true;
}

void Semaphore::lock_shared() {
  LOG_TRACE() << "lock_shared()";
  try_lock_shared_until(Deadline{});
}

void Semaphore::unlock_shared() {
  remaining_simultaneous_locks_.fetch_add(1, std::memory_order_release);

  impl::WaitList::Lock lock{*lock_waiters_};
  lock_waiters_->WakeupOne(lock);
}

bool Semaphore::try_lock_shared() {
  LOG_TRACE() << "try_lock_shared()";
  return LockFastPath();
}

bool Semaphore::try_lock_shared_until(Deadline deadline) {
  LOG_TRACE() << "try_lock_shared_until()";
  return LockFastPath() || LockSlowPath(deadline);
}

size_t Semaphore::RemainingApprox() const {
  return remaining_simultaneous_locks_.load(std::memory_order_relaxed);
}

}  // namespace engine
