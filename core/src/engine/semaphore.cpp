#include <userver/engine/semaphore.hpp>

#include <fmt/format.h>

#include <userver/engine/task/cancel.hpp>
#include <userver/utils/assert.hpp>

#include <engine/impl/wait_list.hpp>
#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace {

class SemaphoreWaitStrategy final : public impl::WaitStrategy {
 public:
  SemaphoreWaitStrategy(impl::WaitList& waiters, impl::TaskContext& current,
                        Deadline deadline) noexcept
      : WaitStrategy(deadline),
        waiters_(waiters),
        current_(current),
        waiter_token_(waiters_),
        lock_(waiters_) {}

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

CancellableSemaphore::CancellableSemaphore(Counter capacity)
    : acquired_locks_(0), capacity_(capacity) {}

CancellableSemaphore::~CancellableSemaphore() {
  UASSERT_MSG(acquired_locks_.load() == 0,
              fmt::format("CancellableSemaphore is destroyed while in use "
                          "(acquired={}, capacity={})",
                          acquired_locks_.load(), capacity_.load()));
}

void CancellableSemaphore::SetCapacity(Counter capacity) {
  capacity_.store(capacity);

  if (lock_waiters_->GetCountOfSleepies()) {
    impl::WaitList::Lock lock{*lock_waiters_};
    lock_waiters_->WakeupAll(lock);
  }
}

CancellableSemaphore::Counter CancellableSemaphore::GetCapacity() const
    noexcept {
  return capacity_.load();
}

std::size_t CancellableSemaphore::RemainingApprox() const {
  const auto acquired = acquired_locks_.load(std::memory_order_relaxed);
  const auto capacity = capacity_.load(std::memory_order_relaxed);
  return capacity >= acquired ? capacity - acquired : 0;
}

std::size_t CancellableSemaphore::UsedApprox() const {
  return acquired_locks_.load(std::memory_order_relaxed);
}

void CancellableSemaphore::lock_shared() { lock_shared_count(1); }

void CancellableSemaphore::lock_shared_count(const Counter count) {
  const bool success = try_lock_shared_until_count(Deadline{}, count);
  if (!success) {
    if (engine::current_task::ShouldCancel()) {
      throw SemaphoreLockCancelledError(
          "Semaphore lock is stopped by task cancellation");
    } else {
      throw UnreachableSemaphoreLockError(fmt::format(
          "The amount of locks requested is greater than CancellableSemaphore "
          "capacity: count={}, capacity={}",
          count, capacity_.load()));
    }
  }
}

void CancellableSemaphore::unlock_shared() { unlock_shared_count(1); }

void CancellableSemaphore::unlock_shared_count(const Counter count) {
  UASSERT(count > 0);

  const auto old_acquired_locks =
      acquired_locks_.fetch_sub(count, std::memory_order_acq_rel);
  UASSERT_MSG(old_acquired_locks >= old_acquired_locks - count,
              fmt::format("Trying to release more locks than have been "
                          "acquired: count={}, acquired={}",
                          count, old_acquired_locks));

  if (lock_waiters_->GetCountOfSleepies()) {
    impl::WaitList::Lock lock{*lock_waiters_};
    if (count > 1) {
      lock_waiters_->WakeupAll(lock);
    } else {
      lock_waiters_->WakeupOne(lock);
    }
  }
}

bool CancellableSemaphore::try_lock_shared() {
  return try_lock_shared_count(1);
}

bool CancellableSemaphore::try_lock_shared_count(const Counter count) {
  return LockFastPath(count) == TryLockStatus::kSuccess;
}

bool CancellableSemaphore::try_lock_shared_until(Deadline deadline) {
  return try_lock_shared_until_count(deadline, 1);
}

bool CancellableSemaphore::try_lock_shared_until_count(Deadline deadline,
                                                       const Counter count) {
  const auto status = LockFastPath(count);
  if (status == TryLockStatus::kSuccess) return true;
  if (status == TryLockStatus::kPermanentFailure) return false;
  return LockSlowPath(deadline, count);
}

bool CancellableSemaphore::LockSlowPath(Deadline deadline,
                                        const Counter count) {
  UASSERT(count > 0);

  auto& current = current_task::GetCurrentTaskContext();
  SemaphoreWaitStrategy wait_manager(*lock_waiters_, current, deadline);

  TryLockStatus status{};
  while ((status = DoTryLock(count)) == TryLockStatus::kTransientFailure) {
    if (current.ShouldCancel()) return false;
    switch (current.Sleep(wait_manager)) {
      case impl::TaskContext::WakeupSource::kDeadlineTimer:
      case impl::TaskContext::WakeupSource::kCancelRequest:
        return false;
      default:;
    }
  }
  return (status == TryLockStatus::kSuccess);
}

CancellableSemaphore::TryLockStatus CancellableSemaphore::DoTryLock(
    const Counter count) {
  auto capacity = capacity_.load(std::memory_order_acquire);
  if (count > capacity) return TryLockStatus::kPermanentFailure;

  auto expected = acquired_locks_.load(std::memory_order_acquire);
  bool success = false;

  while (expected <= capacity - count && !success) {
    success = acquired_locks_.compare_exchange_weak(expected, expected + count,
                                                    std::memory_order_relaxed);
  }

  return success ? TryLockStatus::kSuccess : TryLockStatus::kTransientFailure;
}

CancellableSemaphore::TryLockStatus CancellableSemaphore::LockFastPath(
    const Counter count) {
  UASSERT(count > 0);

  const auto status = DoTryLock(count);
  return status;
}

Semaphore::Semaphore(Counter capacity) : sem_(capacity) {}

Semaphore::~Semaphore() = default;

void Semaphore::SetCapacity(Counter capacity) { sem_.SetCapacity(capacity); }

Semaphore::Counter Semaphore::GetCapacity() const noexcept {
  return sem_.GetCapacity();
}

std::size_t Semaphore::RemainingApprox() const {
  return sem_.RemainingApprox();
}

std::size_t Semaphore::UsedApprox() const { return sem_.UsedApprox(); }

void Semaphore::lock_shared() {
  engine::TaskCancellationBlocker blocker;
  sem_.lock_shared();
}

void Semaphore::unlock_shared() { sem_.unlock_shared(); }

bool Semaphore::try_lock_shared() {
  engine::TaskCancellationBlocker blocker;
  return sem_.try_lock_shared();
}

bool Semaphore::try_lock_shared_until(Deadline deadline) {
  engine::TaskCancellationBlocker blocker;
  return sem_.try_lock_shared_until(deadline);
}

void Semaphore::lock_shared_count(Counter count) {
  engine::TaskCancellationBlocker blocker;
  sem_.lock_shared_count(count);
}

void Semaphore::unlock_shared_count(Counter count) {
  sem_.unlock_shared_count(count);
}

bool Semaphore::try_lock_shared_count(Counter count) {
  engine::TaskCancellationBlocker blocker;
  return sem_.try_lock_shared_count(count);
}

bool Semaphore::try_lock_shared_until_count(Deadline deadline, Counter count) {
  engine::TaskCancellationBlocker blocker;
  return sem_.try_lock_shared_until_count(deadline, count);
}

SemaphoreLock::SemaphoreLock(Semaphore& sem) : sem_(&sem), owns_lock_(true) {
  sem_->lock_shared();
}

SemaphoreLock::SemaphoreLock(Semaphore& sem, std::defer_lock_t) noexcept
    : sem_(&sem) {}

SemaphoreLock::SemaphoreLock(Semaphore& sem, std::try_to_lock_t) : sem_(&sem) {
  TryLock();
}

SemaphoreLock::SemaphoreLock(Semaphore& sem, std::adopt_lock_t) noexcept
    : sem_(&sem), owns_lock_(true) {}

SemaphoreLock::SemaphoreLock(Semaphore& sem, Deadline deadline) : sem_(&sem) {
  TryLockUntil(deadline);
}

SemaphoreLock& SemaphoreLock::operator=(SemaphoreLock&& other) noexcept {
  if (OwnsLock()) Unlock();
  sem_ = other.sem_;
  owns_lock_ = other.owns_lock_;
  other.owns_lock_ = false;

  return *this;
}

SemaphoreLock::SemaphoreLock(SemaphoreLock&& other) noexcept
    : sem_(other.sem_), owns_lock_(std::exchange(other.owns_lock_, false)) {}

SemaphoreLock::~SemaphoreLock() {
  if (OwnsLock()) Unlock();
}

bool SemaphoreLock::OwnsLock() const noexcept { return owns_lock_; }

void SemaphoreLock::Lock() {
  UASSERT(sem_);
  UASSERT(!owns_lock_);
  sem_->lock_shared();
}

bool SemaphoreLock::TryLock() {
  UASSERT(sem_);
  UASSERT(!owns_lock_);
  owns_lock_ = sem_->try_lock_shared();
  return owns_lock_;
}

bool SemaphoreLock::TryLockUntil(Deadline deadline) {
  UASSERT(sem_);
  UASSERT(!owns_lock_);
  owns_lock_ = sem_->try_lock_shared_until(deadline);
  return owns_lock_;
}

void SemaphoreLock::Unlock() {
  UASSERT(sem_);
  UASSERT(owns_lock_);
  sem_->unlock_shared();
  owns_lock_ = false;
}

void SemaphoreLock::Release() {
  sem_ = nullptr;
  owns_lock_ = false;
}

}  // namespace engine

USERVER_NAMESPACE_END
