#include "mutex.hpp"

#include <cassert>

#include "task/task_context.hpp"

namespace engine {

Mutex::Mutex() : lock_waiters_(std::make_shared<WaitList>()), owner_(nullptr) {}

Mutex::~Mutex() { assert(!owner_); }

void Mutex::lock() {
  impl::TaskContext* const current = current_task::GetCurrentTaskContext();
  assert(current);
  WaitList::Lock lock(*lock_waiters_);
  while (owner_) {
    assert(owner_ != current);
    impl::TaskContext::SleepParams sleep_params;
    sleep_params.wait_list = lock_waiters_;
    sleep_params.exec_after_asleep = [this, &lock, current] {
      lock_waiters_->Append(lock, current);
      lock.Release();
    };
    sleep_params.exec_before_awake = [&lock] { lock.Acquire(); };
    current->Sleep(std::move(sleep_params));
  }
  owner_ = current;
}

void Mutex::unlock() {
  WaitList::Lock lock(*lock_waiters_);
  assert(owner_ == current_task::GetCurrentTaskContext());
  owner_ = nullptr;
  lock_waiters_->WakeupOne(lock);
}

}  // namespace engine
