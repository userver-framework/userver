#pragma once

#include <atomic>

#include <userver/engine/deadline.hpp>

#include <userver/utils/assert.hpp>

#include <engine/impl/wait_list.hpp>
#include <engine/impl/wait_list_light.hpp>
#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

template <class Waiters>
class MutexImpl final {
 public:
  MutexImpl();
  ~MutexImpl();

  MutexImpl(const MutexImpl&) = delete;
  MutexImpl(MutexImpl&&) = delete;
  MutexImpl& operator=(const MutexImpl&) = delete;
  MutexImpl& operator=(MutexImpl&&) = delete;

  void lock();

  void unlock();

  bool try_lock();

  bool try_lock_until(Deadline deadline);

 private:
  class MutexWaitStrategy;

  bool LockFastPath(TaskContext&);
  bool LockSlowPath(TaskContext&, Deadline);

  std::atomic<TaskContext*> owner_;
  Waiters lock_waiters_;
};



template<>
class MutexImpl<WaitList>::MutexWaitStrategy final : public WaitStrategy {
 public:
  MutexWaitStrategy(WaitList& waiters, TaskContext& current,
                    Deadline deadline)
      : WaitStrategy(deadline),
        waiters_(waiters),
        current_(current),
        waiter_token_(waiters),
        lock_(waiters) {}

  void SetupWakeups() override {
    waiters_.Append(lock_, &current_);
    lock_.unlock();
  }

  void DisableWakeups() override {
    lock_.lock();
    waiters_.Remove(lock_, current_);
  }

 private:
  WaitList& waiters_;
  TaskContext& current_;
  const WaitList::WaitersScopeCounter waiter_token_;
  WaitList::Lock lock_;
};

template<>
class MutexImpl<WaitListLight>::MutexWaitStrategy final : public WaitStrategy {
 public:
  MutexWaitStrategy(WaitListLight& waiters, TaskContext& current,
                    Deadline deadline)
      : WaitStrategy(deadline),
        waiters_(waiters),
        current_(current) {}

  void SetupWakeups() override {
    waiters_.Append(&current_);
  }

  void DisableWakeups() override {
    waiters_.Remove(current_);
  }

 private:
  WaitListLight& waiters_;
  TaskContext& current_;
};

template <class Waiters>
MutexImpl<Waiters>::MutexImpl() : owner_(nullptr) {}

template <class Waiters>
MutexImpl<Waiters>::~MutexImpl() { UASSERT(!owner_); }

template <class Waiters>
bool MutexImpl<Waiters>::LockFastPath(TaskContext& current) {
  TaskContext* expected = nullptr;
  return owner_.compare_exchange_strong(expected, &current,
                                        std::memory_order_acquire);
}

template <class Waiters>
bool MutexImpl<Waiters>::LockSlowPath(TaskContext& current, Deadline deadline) {
  TaskContext* expected = nullptr;

  engine::TaskCancellationBlocker block_cancels;
  MutexWaitStrategy wait_manager(lock_waiters_, current, deadline);
  while (!owner_.compare_exchange_strong(expected, &current,
                                         std::memory_order_relaxed)) {
    UINVARIANT(expected != &current,
               "MutexImpl is locked twice from the same task");

    if (current.Sleep(wait_manager) ==
        TaskContext::WakeupSource::kDeadlineTimer) {
      return false;
    }

    expected = nullptr;
  }
  return true;
}

template <class Waiters>
void MutexImpl<Waiters>::lock() { try_lock_until(Deadline{}); }

template <class Waiters>
void MutexImpl<Waiters>::unlock() {
  auto* old_owner = owner_.exchange(nullptr, std::memory_order_acq_rel);
  UASSERT(old_owner && old_owner->IsCurrent());

  if constexpr (std::is_same<Waiters, WaitList>::value) {
    if (lock_waiters_.GetCountOfSleepies()) {
      WaitList::Lock lock(lock_waiters_);
      lock_waiters_.WakeupOne(lock);
    }
  }

  if constexpr (std::is_same<Waiters, WaitListLight>::value) {
    lock_waiters_.WakeupOne();
  }
}

template <class Waiters>
bool MutexImpl<Waiters>::try_lock() {
  auto& current = current_task::GetCurrentTaskContext();
  return LockFastPath(current);
}

template <class Waiters>
bool MutexImpl<Waiters>::try_lock_until(Deadline deadline) {
  auto& current = current_task::GetCurrentTaskContext();
  return LockFastPath(current) || LockSlowPath(current, deadline);
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
