#include <engine/mutex.hpp>

#include <utils/assert.hpp>
#include "task/task_context.hpp"
#include "wait_list.hpp"

namespace engine {

namespace impl {
namespace {
class MutexWaitStrategy final : public WaitStrategy {
 public:
  MutexWaitStrategy(const std::shared_ptr<WaitList>& waiters,
                    TaskContext* current, Deadline deadline)
      : WaitStrategy(deadline),
        waiters_(waiters),
        current_(current),
        lock_(*waiters) {}

  void AfterAsleep() override {
    waiters_->Append(lock_, current_);
    lock_.Release();
  }

  void BeforeAwake() override { lock_.Acquire(); }

  std::shared_ptr<WaitListBase> GetWaitList() override { return waiters_; }

 private:
  const std::shared_ptr<WaitList>& waiters_;
  TaskContext* const current_;
  WaitList::Lock lock_;
};
}  // namespace
}  // namespace impl

Mutex::Mutex()
    : lock_waiters_(std::make_shared<impl::WaitList>()), owner_(nullptr) {}

Mutex::~Mutex() { UASSERT(!owner_); }

bool Mutex::LockFastPath(impl::TaskContext* current) {
  impl::TaskContext* expected = nullptr;
  return owner_.compare_exchange_strong(expected, current,
                                        std::memory_order_acquire);
}

bool Mutex::LockSlowPath(impl::TaskContext* current, Deadline deadline) {
  impl::TaskContext* expected = nullptr;

  impl::MutexWaitStrategy wait_manager(lock_waiters_, current, deadline);
  while (!owner_.compare_exchange_strong(expected, current,
                                         std::memory_order_relaxed)) {
    if (expected == current) {
      UASSERT_MSG(false, "Mutex is locked twice from the same task");
      throw std::runtime_error("Mutex is locked twice from the same task");
    }

    current->Sleep(&wait_manager);
    if (current->GetWakeupSource() ==
        impl::TaskContext::WakeupSource::kDeadlineTimer) {
      return false;
    }

    expected = nullptr;
  }
  return true;
}

void Mutex::lock() { try_lock_until(Deadline{}); }

void Mutex::unlock() {
  [[maybe_unused]] const auto old_owner =
      owner_.exchange(nullptr, std::memory_order_release);
  UASSERT(old_owner == current_task::GetCurrentTaskContext());

  impl::WaitList::Lock lock(*lock_waiters_);
  lock_waiters_->WakeupOne(lock);
}

bool Mutex::try_lock() {
  auto* current = current_task::GetCurrentTaskContext();
  return LockFastPath(current);
}

bool Mutex::try_lock_until(Deadline deadline) {
  auto* current = current_task::GetCurrentTaskContext();
  return LockFastPath(current) || LockSlowPath(current, deadline);
}

}  // namespace engine
