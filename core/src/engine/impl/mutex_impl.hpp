#pragma once

#include <atomic>

#include <userver/engine/deadline.hpp>

#include <userver/utils/assert.hpp>

#include <engine/impl/wait_list.hpp>
#include <engine/impl/wait_list_light.hpp>
#include <engine/task/task_context.hpp>
#include <userver/compiler/impl/tsan.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

template <class Waiters>
class MutexImpl {
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

  bool LockFastPath(TaskContext&) noexcept;
  bool LockSlowPath(TaskContext&, Deadline);

  std::atomic<TaskContext*> owner_;
  Waiters lock_waiters_;
};

template <>
class MutexImpl<WaitList>::MutexWaitStrategy final : public WaitStrategy {
 public:
  MutexWaitStrategy(MutexImpl<WaitList>& mutex, TaskContext& current)
      : mutex_(mutex),
        current_(current),
        waiter_token_(mutex_.lock_waiters_),
        lock_(mutex_.lock_waiters_) {}

  EarlyWakeup SetupWakeups() override {
    mutex_.lock_waiters_.Append(lock_, &current_);
    lock_.unlock();
    // A race is not possible here, because check + Append is performed under
    // WaitList::Lock, and notification also takes WaitList::Lock.
    return EarlyWakeup{false};
  }

  void DisableWakeups() noexcept override {
    lock_.lock();
    mutex_.lock_waiters_.Remove(lock_, current_);
  }

 private:
  MutexImpl<WaitList>& mutex_;
  TaskContext& current_;
  const WaitList::WaitersScopeCounter waiter_token_;
  WaitList::Lock lock_;
};

template <>
class MutexImpl<WaitListLight>::MutexWaitStrategy final : public WaitStrategy {
 public:
  MutexWaitStrategy(MutexImpl<WaitListLight>& mutex, TaskContext& current)
      : mutex_(mutex), current_(current) {}

  EarlyWakeup SetupWakeups() override {
    mutex_.lock_waiters_.Append(&current_);
    if (mutex_.owner_.load() == nullptr) {
      mutex_.lock_waiters_.Remove(current_);
      return EarlyWakeup{true};
    }
    return EarlyWakeup{false};
  }

  void DisableWakeups() noexcept override {
    mutex_.lock_waiters_.Remove(current_);
  }

 private:
  MutexImpl<WaitListLight>& mutex_;
  TaskContext& current_;
};

template <class Waiters>
MutexImpl<Waiters>::MutexImpl() : owner_(nullptr) {
#if USERVER_IMPL_HAS_TSAN
  __tsan_mutex_create(this, __tsan_mutex_not_static);
#endif
}

template <class Waiters>
MutexImpl<Waiters>::~MutexImpl() {
  UASSERT(!owner_);
#if USERVER_IMPL_HAS_TSAN
  __tsan_mutex_destroy(this, __tsan_mutex_not_static);
#endif
}

template <class Waiters>
bool MutexImpl<Waiters>::LockFastPath(TaskContext& current) noexcept {
  TaskContext* expected = nullptr;
  return owner_.compare_exchange_strong(expected, &current,
                                        std::memory_order_acquire);
}

template <class Waiters>
bool MutexImpl<Waiters>::LockSlowPath(TaskContext& current, Deadline deadline) {
  TaskContext* expected = nullptr;

  const engine::TaskCancellationBlocker block_cancels;
  MutexWaitStrategy wait_manager{*this, current};
  while (!owner_.compare_exchange_strong(expected, &current,
                                         std::memory_order_relaxed)) {
    UINVARIANT(expected != &current,
               "MutexImpl is locked twice from the same task");

    const auto wakeup_source = current.Sleep(wait_manager, deadline);
    if (!HasWaitSucceeded(wakeup_source)) {
      return false;
    }

    expected = nullptr;
  }
  return true;
}

template <class Waiters>
void MutexImpl<Waiters>::lock() {
  try_lock_until(Deadline{});
}

template <class Waiters>
void MutexImpl<Waiters>::unlock() {
#if USERVER_IMPL_HAS_TSAN
  __tsan_mutex_pre_unlock(this, 0);
#endif
  auto* old_owner = owner_.exchange(nullptr, std::memory_order_acq_rel);
  UASSERT(old_owner && old_owner->IsCurrent());

  if constexpr (std::is_same_v<Waiters, WaitList>) {
    if (lock_waiters_.GetCountOfSleepies()) {
      WaitList::Lock lock(lock_waiters_);
      lock_waiters_.WakeupOne(lock);
    }
  } else {
    static_assert(std::is_same_v<Waiters, WaitListLight>);
    lock_waiters_.WakeupOne();
  }

#if USERVER_IMPL_HAS_TSAN
  __tsan_mutex_post_unlock(this, 0);
#endif
}

template <class Waiters>
bool MutexImpl<Waiters>::try_lock() {
#if USERVER_IMPL_HAS_TSAN
  __tsan_mutex_pre_lock(this, __tsan_mutex_try_lock);
#endif

  auto& current = current_task::GetCurrentTaskContext();
  const auto result = LockFastPath(current);

#if USERVER_IMPL_HAS_TSAN
  __tsan_mutex_post_lock(
      this, __tsan_mutex_try_lock | (result ? 0 : __tsan_mutex_try_lock_failed),
      0);
#endif

  return result;
}

template <class Waiters>
bool MutexImpl<Waiters>::try_lock_until(Deadline deadline) {
#if USERVER_IMPL_HAS_TSAN
  __tsan_mutex_pre_lock(this, __tsan_mutex_try_lock);
#endif

  auto& current = current_task::GetCurrentTaskContext();
  const auto result = LockFastPath(current) || LockSlowPath(current, deadline);

#if USERVER_IMPL_HAS_TSAN
  __tsan_mutex_post_lock(
      this, __tsan_mutex_try_lock | (result ? 0 : __tsan_mutex_try_lock_failed),
      0);
#endif

  return result;
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
