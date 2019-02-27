#include <engine/semaphore.hpp>

#include <engine/task/cancel.hpp>
#include "task/task_context.hpp"

namespace engine {

namespace impl {
namespace {
class SemaphoreWaitPolicy final : public WaitStrategy {
 public:
  SemaphoreWaitPolicy(const std::shared_ptr<impl::WaitList>& waiters,
                      TaskContext* current)
      : WaitStrategy({}),
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

void Semaphore::lock() {
  impl::TaskContext* const current = current_task::GetCurrentTaskContext();
  LOG_TRACE() << "lock()";

  auto expected = remaining_simultaneous_locks_.load(std::memory_order_acquire);
  if (expected == 0) expected = 1;

  if (remaining_simultaneous_locks_.compare_exchange_strong(
          expected, expected - 1, std::memory_order_relaxed)) {
    LOG_TRACE() << "exit via fast path";
  } else {
    if (expected == 0) expected = 1;

    impl::SemaphoreWaitPolicy wait_manager(lock_waiters_, current);
    while (!remaining_simultaneous_locks_.compare_exchange_weak(
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
    }

    LOG_TRACE() << "exit";
  }
}

void Semaphore::unlock() {
  remaining_simultaneous_locks_.fetch_add(1, std::memory_order_release);

  impl::WaitList::Lock lock{*lock_waiters_};
  lock_waiters_->WakeupOne(lock);
}

}  // namespace engine
