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

void Mutex::lock() {
  impl::TaskContext* const current = current_task::GetCurrentTaskContext();
  assert(current);

  impl::MutexWaitStrategy wait_manager(lock_waiters_, current);
  while (owner_) {
    assert(owner_ != current);
    current->Sleep(&wait_manager);
  }
  owner_ = current;
}

void Mutex::unlock() {
  impl::WaitList::Lock lock(*lock_waiters_);
  assert(owner_ == current_task::GetCurrentTaskContext());
  owner_ = nullptr;
  lock_waiters_->WakeupOne(lock);
}

}  // namespace engine
