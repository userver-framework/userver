#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <shared_mutex>  // for std locks

#include <engine/deadline.hpp>

namespace engine {

namespace impl {
class WaitList;
}  // namespace impl

/// Class that allows `max_simultaneous_locks` concurrent accesses to the
/// critical section.
///
/// Example:
///   engine::Semaphore sem{2};
///   std::unique_lock<engine::Semaphore> lock{sem};
///   utils::Async("work", [&sem]() {
///       std::lock_guard<engine::Semaphore> guard{sem};
///       // ...
///   }).Detach();
class Semaphore {
 public:
  explicit Semaphore(std::size_t max_simultaneous_locks);
  ~Semaphore() = default;

  Semaphore(Semaphore&&) = delete;
  Semaphore(const Semaphore&) = delete;
  Semaphore& operator=(Semaphore&&) = delete;
  Semaphore& operator=(const Semaphore&) = delete;

  void lock_shared();
  void unlock_shared();
  bool try_lock_shared();

  template <typename Rep, typename Period>
  bool try_lock_shared_for(const std::chrono::duration<Rep, Period>&);

  template <typename Clock, typename Duration>
  bool try_lock_shared_until(const std::chrono::time_point<Clock, Duration>&);

  bool try_lock_shared_until(Deadline deadline);

 private:
  bool LockFastPath();
  bool LockSlowPath(Deadline);

  std::shared_ptr<impl::WaitList> lock_waiters_;
  std::atomic<std::size_t> remaining_simultaneous_locks_;
};

template <typename Rep, typename Period>
bool Semaphore::try_lock_shared_for(
    const std::chrono::duration<Rep, Period>& duration) {
  return try_lock_shared_until(Deadline::FromDuration(duration));
}

template <typename Clock, typename Duration>
bool Semaphore::try_lock_shared_until(
    const std::chrono::time_point<Clock, Duration>& until) {
  return try_lock_shared_until(Deadline::FromTimePoint(until));
}

}  // namespace engine
