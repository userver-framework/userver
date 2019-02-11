#include <engine/mutex.hpp>

#include <cassert>

#include "task/task_context.hpp"
#include "wait_list.hpp"

namespace engine {

namespace impl {
namespace {
class MutexWaitStrategy final : public WaitStrategy {
 public:
  MutexWaitStrategy(std::shared_ptr<WaitList> waiters, TaskContext* current)
      : WaitStrategy({}),
        waiters_(waiters),
        lock_(*waiters),
        current_(current) {}

  void AfterAsleep() override {
    waiters_->Append(lock_, current_);
    lock_.Release();
  }

  void BeforeAwake() override { lock_.Acquire(); }

  std::shared_ptr<WaitListBase> GetWaitList() override { return waiters_; }

 private:
  const std::shared_ptr<WaitList> waiters_;
  WaitList::Lock lock_;
  TaskContext* const current_;
};
}  // namespace

}  // namespace impl

Mutex::Mutex()
    : lock_waiters_(std::make_shared<impl::WaitList>()), owner_(nullptr) {}

Mutex::~Mutex() { assert(!owner_); }

void Mutex::LockSlowPath(impl::TaskContext* const current) {
  impl::TaskContext* expected = nullptr;

  impl::MutexWaitStrategy wait_manager(lock_waiters_, current);
  while (!owner_.compare_exchange_strong(expected, current,
                                         std::memory_order_relaxed)) {
    if (expected == current) {
      assert(false && "Mutex is locked twice from the same task");
      throw std::runtime_error("Mutex is locked twice from the same task");
    }
    current->Sleep(&wait_manager);
    expected = nullptr;
  };
}

void Mutex::lock() {
  impl::TaskContext* const current = current_task::GetCurrentTaskContext();

  impl::TaskContext* expected = nullptr;
  if (owner_.compare_exchange_strong(expected, current,
                                     std::memory_order_acquire)) {
    return;  // Fast path
  } else {
    LockSlowPath(current);
  }
}

void Mutex::unlock() {
  [[maybe_unused]] const auto old_owner =
      owner_.exchange(nullptr, std::memory_order_release);
  assert(old_owner == current_task::GetCurrentTaskContext());

  impl::WaitList::Lock lock(*lock_waiters_);
  lock_waiters_->WakeupOne(lock);
}

}  // namespace engine
