#include <userver/engine/mutex.hpp>

#include <userver/utils/assert.hpp>

#include <engine/impl/wait_list.hpp>
#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

class Mutex::MutexWaitStrategy final : public impl::WaitStrategy {
 public:
  MutexWaitStrategy(Mutex& mutex, impl::TaskContext& current, Deadline deadline)
      : WaitStrategy(deadline),
        mutex_(mutex),
        wait_scope_(*mutex_.lock_waiters_, current) {}

  void SetupWakeups() override {
    wait_scope_.Append();
    if (mutex_.owner_.load(std::memory_order_acquire) == nullptr) {
      wait_scope_.GetContext().Wakeup(
          impl::TaskContext::WakeupSource::kWaitList,
          impl::TaskContext::NoEpoch{});
    }
  }

  void DisableWakeups() override { wait_scope_.Remove(); }

 private:
  Mutex& mutex_;
  impl::WaitScope wait_scope_;
};

Mutex::Mutex() : owner_(nullptr) {}

Mutex::~Mutex() { UASSERT(!owner_); }

bool Mutex::LockFastPath(impl::TaskContext& current) {
  impl::TaskContext* expected = nullptr;
  return owner_.compare_exchange_strong(expected, &current,
                                        std::memory_order_acquire);
}

bool Mutex::LockSlowPath(impl::TaskContext& current, Deadline deadline) {
  impl::TaskContext* expected = nullptr;

  engine::TaskCancellationBlocker block_cancels;
  MutexWaitStrategy wait_manager(*this, current, deadline);
  while (!owner_.compare_exchange_strong(expected, &current,
                                         std::memory_order_relaxed)) {
    UINVARIANT(expected != &current,
               "Mutex is locked twice from the same task");

    if (current.Sleep(wait_manager) ==
        impl::TaskContext::WakeupSource::kDeadlineTimer) {
      return false;
    }

    expected = nullptr;
  }
  return true;
}

void Mutex::lock() { try_lock_until(Deadline{}); }

void Mutex::unlock() {
  auto* old_owner = owner_.exchange(nullptr, std::memory_order_acq_rel);
  UASSERT(old_owner && old_owner->IsCurrent());

  lock_waiters_->WakeupOne();
}

bool Mutex::try_lock() {
  auto& current = current_task::GetCurrentTaskContext();
  return LockFastPath(current);
}

bool Mutex::try_lock_until(Deadline deadline) {
  auto& current = current_task::GetCurrentTaskContext();
  return LockFastPath(current) || LockSlowPath(current, deadline);
}

}  // namespace engine

USERVER_NAMESPACE_END
