#include <userver/engine/shared_mutex.hpp>

#include <userver/engine/task/cancel.hpp>
#include <userver/utils/scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace {
constexpr auto kWriterLock = std::numeric_limits<Semaphore::Counter>::max();
}

SharedMutex::SharedMutex()
    : semaphore_(kWriterLock), waiting_writers_count_(0) {}

void SharedMutex::lock() { try_lock_until(Deadline{}); }

void SharedMutex::unlock() {
  utils::ScopeGuard stop_wait([this] { DecWaitingWriters(); });

  semaphore_.unlock_shared_count(kWriterLock);
}

void SharedMutex::DecWaitingWriters() {
  /*
   * If we're the last writer, notify readers.
   * If we're not the last, do nothing: readers are still waiting,
   * the next writer will eventually lock the semaphore.
   */
  auto writers_left =
      waiting_writers_count_.fetch_sub(1, std::memory_order_relaxed);
  UASSERT_MSG(writers_left > 0, "unlock without lock");
  if (writers_left == 1) {
    engine::TaskCancellationBlocker blocker;
    std::lock_guard<Mutex> lock(waiting_writers_count_mutex_);
    waiting_writers_count_cv_.NotifyAll();
  }
}

bool SharedMutex::try_lock_until(Deadline deadline) {
  waiting_writers_count_.fetch_add(1, std::memory_order_relaxed);

  utils::ScopeGuard stop_wait([this] { DecWaitingWriters(); });
  if (semaphore_.try_lock_shared_until_count(deadline, kWriterLock)) {
    stop_wait.Release();
    return true;
  }
  return false;
}

bool SharedMutex::try_lock() { return try_lock_until(Deadline::Passed()); }

void SharedMutex::lock_shared() {
  WaitForNoWaitingWriters(Deadline{});

  /*
   * There is a deliberate TOCTOU race between "wait for no writers" and
   * "ok, now let's lock" - it's just a cheap way to avoid writers starvation.
   * If one or two readers sneak just before a writer out of turn,
   * we just don't care.
   */

  semaphore_.lock_shared();
}

void SharedMutex::unlock_shared() { semaphore_.unlock_shared(); }

bool SharedMutex::try_lock_shared() {
  if (HasWaitingWriter()) return false;
  return semaphore_.try_lock_shared();
}

bool SharedMutex::try_lock_shared_until(Deadline deadline) {
  if (!WaitForNoWaitingWriters(deadline)) return false;

  /* Same deliberate race, see comment in lock_shared() */
  return semaphore_.try_lock_shared_until(deadline);
}

bool SharedMutex::HasWaitingWriter() const noexcept {
  return waiting_writers_count_.load() > 0;
}

bool SharedMutex::WaitForNoWaitingWriters(Deadline deadline) {
  /* Fast path */
  if (waiting_writers_count_ == 0) return true;

  std::unique_lock<Mutex> lock(waiting_writers_count_mutex_);
  return waiting_writers_count_cv_.WaitUntil(
      lock, deadline, [this] { return waiting_writers_count_ == 0; });
}

}  // namespace engine

USERVER_NAMESPACE_END
