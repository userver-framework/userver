#include <userver/engine/mutex.hpp>

#include <userver/utils/assert.hpp>

#include <engine/impl/wait_list.hpp>
#include <engine/task/task_context.hpp>

#ifdef BOOST_USE_TSAN
#include <sanitizer/tsan_interface.h>
#endif

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace {

class MutexWaitStrategy final : public impl::WaitStrategy {
 public:
  MutexWaitStrategy(impl::WaitList& waiters, impl::TaskContext& current,
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
  impl::WaitList& waiters_;
  impl::TaskContext& current_;
  const impl::WaitList::WaitersScopeCounter waiter_token_;
  impl::WaitList::Lock lock_;
};

}  // namespace

Mutex::Mutex() : owner_(nullptr) {
#if defined(BOOST_USE_TSAN)
  __tsan_mutex_create(this, 0);
#endif
}

Mutex::~Mutex() {
  UASSERT(!owner_);
#if defined(BOOST_USE_TSAN)
  __tsan_mutex_destroy(this, 0);
#endif
}

bool Mutex::LockFastPath(impl::TaskContext& current) {
#if defined(BOOST_USE_TSAN)
  __tsan_mutex_pre_lock(this, __tsan_mutex_try_lock);
#endif
  impl::TaskContext* expected = nullptr;
  const auto result = owner_.compare_exchange_strong(expected, &current,
                                                     std::memory_order_acquire);
#if defined(BOOST_USE_TSAN)
  __tsan_mutex_post_lock(
      this, result ? __tsan_mutex_try_lock : __tsan_mutex_try_lock_failed, 0);
#endif
  return result;
}

bool Mutex::LockSlowPath(impl::TaskContext& current, Deadline deadline) {
#if defined(BOOST_USE_TSAN)
  __tsan_mutex_pre_lock(this, 0);
#endif
  impl::TaskContext* expected = nullptr;

  engine::TaskCancellationBlocker block_cancels;
  MutexWaitStrategy wait_manager(*lock_waiters_, current, deadline);
  while (!owner_.compare_exchange_strong(expected, &current,
                                         std::memory_order_relaxed)) {
    UINVARIANT(expected != &current,
               "Mutex is locked twice from the same task");

    if (current.Sleep(wait_manager) ==
        impl::TaskContext::WakeupSource::kDeadlineTimer) {
#if defined(BOOST_USE_TSAN)
      __tsan_mutex_post_lock(this, 0, 0);
      __tsan_mutex_pre_unlock(this, 0);
      __tsan_mutex_post_unlock(this, 0);
#endif
      return false;
    }

    expected = nullptr;
  }
#if defined(BOOST_USE_TSAN)
  __tsan_mutex_post_lock(this, 0, 0);
#endif
  return true;
}

void Mutex::lock() { try_lock_until(Deadline{}); }

void Mutex::unlock() {
#if defined(BOOST_USE_TSAN)
  __tsan_mutex_pre_unlock(this, 0);
#endif
  auto* old_owner = owner_.exchange(nullptr, std::memory_order_acq_rel);
  UASSERT(old_owner && old_owner->IsCurrent());

  if (lock_waiters_->GetCountOfSleepies()) {
    impl::WaitList::Lock lock(*lock_waiters_);
    lock_waiters_->WakeupOne(lock);
  }
#if defined(BOOST_USE_TSAN)
  __tsan_mutex_post_unlock(this, 0);
#endif
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
