#include <engine/semaphore.hpp>

#include <engine/task/cancel.hpp>
#include "task/task_context.hpp"
#include "wait_list.hpp"

namespace engine {

namespace impl {
namespace {
class SemaphoreWaitPolicy final : public WaitStrategy {
 public:
  SemaphoreWaitPolicy(impl::WaitList& waiters, TaskContext* current,
                      Deadline deadline)
      : WaitStrategy(deadline),
        waiters_(waiters),
        current_(current),
        waiter_token_(waiters_),
        lock_{waiters_} {
    UASSERT(current_);
  }

  void AfterAsleep() override {
    waiters_.Append(lock_, current_);
    lock_.Release();
  }

  void BeforeAwake() override { lock_.Acquire(); }

  WaitListBase* GetWaitList() override { return &waiters_; }

 private:
  impl::WaitList& waiters_;
  TaskContext* const current_;
  const WaitList::WaitersScopeCounter waiter_token_;
  WaitList::Lock lock_;
};
}  // namespace
}  // namespace impl

Semaphore::Semaphore(Counter max_simultaneous_locks)
    : lock_waiters_(),
      remaining_simultaneous_locks_(max_simultaneous_locks),
      max_simultaneous_locks_(max_simultaneous_locks),
      is_multi_(false) {
  UASSERT(max_simultaneous_locks > 0);
}

Semaphore::~Semaphore() {
  UASSERT_MSG(
      max_simultaneous_locks_ == remaining_simultaneous_locks_,
      "Semaphore is destroyed while users still holding the lock (max=" +
          std::to_string(max_simultaneous_locks_) +
          ", current=" + std::to_string(remaining_simultaneous_locks_) + ")");
}

bool Semaphore::LockFastPath(const Counter count) {
  UASSERT(count > 0);
  UASSERT(count <= max_simultaneous_locks_);
  if (count > 1) is_multi_ = true;

  LOG_TRACE() << "trying fast path";
  auto expected = remaining_simultaneous_locks_.load(std::memory_order_acquire);
  if (expected < count) expected = count;

  if (remaining_simultaneous_locks_.compare_exchange_strong(
          expected, expected - count, std::memory_order_relaxed)) {
    LOG_TRACE() << "fast path succeeded";
    return true;
  }
  return false;
}

bool Semaphore::LockSlowPath(Deadline deadline, const Counter count) {
  UASSERT(count > 0);
  UASSERT(count <= max_simultaneous_locks_);

  LOG_TRACE() << "trying slow path";

  engine::TaskCancellationBlocker block_cancels;
  auto* current = current_task::GetCurrentTaskContext();
  impl::SemaphoreWaitPolicy wait_manager(*lock_waiters_, current, deadline);

  auto expected = remaining_simultaneous_locks_.load(std::memory_order_relaxed);
  if (expected < count) expected = count;

  while (!remaining_simultaneous_locks_.compare_exchange_strong(
      expected, expected - count, std::memory_order_relaxed)) {
    LOG_TRACE() << "iteration()";
    if (expected > count) {
      // We may acquire the lock, but the `expected` before CAS had an
      // outdated value.
      continue;
    }

    // `expected` < count, expecting count to appear soon.
    expected = count;

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

void Semaphore::unlock_shared() { unlock_shared_count(1); }

void Semaphore::unlock_shared_count(const Counter count) {
  LOG_TRACE() << "unlock_shared()";
  remaining_simultaneous_locks_.fetch_add(count, std::memory_order_acq_rel);

  if (lock_waiters_->GetCountOfSleepies()) {
    impl::WaitList::Lock lock{*lock_waiters_};
    if (is_multi_) {
      lock_waiters_->WakeupAll(lock);
    } else {
      lock_waiters_->WakeupOne(lock);
    }
  }
}

bool Semaphore::try_lock_shared() { return try_lock_shared_count(1); }

bool Semaphore::try_lock_shared_count(const Counter count) {
  LOG_TRACE() << "try_lock_shared()";
  return LockFastPath(count);
}

bool Semaphore::try_lock_shared_until(Deadline deadline) {
  return try_lock_shared_until_count(deadline, 1);
}

bool Semaphore::try_lock_shared_until_count(Deadline deadline,
                                            const Counter count) {
  LOG_TRACE() << "try_lock_shared_until()";
  return LockFastPath(count) || LockSlowPath(deadline, count);
}

size_t Semaphore::RemainingApprox() const {
  return remaining_simultaneous_locks_.load(std::memory_order_relaxed);
}

}  // namespace engine
