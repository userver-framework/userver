#pragma once

/// @file engine/mutex.hpp
/// @brief @copybrief engine::Mutex

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>  // for std locks

#include <engine/deadline.hpp>
#include <engine/wait_list_fwd.hpp>

namespace engine {

/// std::mutex replacement for asynchronous tasks
class Mutex final {
 public:
  Mutex();
  ~Mutex();

  Mutex(const Mutex&) = delete;
  Mutex(Mutex&&) = delete;
  Mutex& operator=(const Mutex&) = delete;
  Mutex& operator=(Mutex&&) = delete;

  void lock();
  void unlock();
  bool try_lock();

  template <typename Rep, typename Period>
  bool try_lock_for(const std::chrono::duration<Rep, Period>&);

  template <typename Clock, typename Duration>
  bool try_lock_until(const std::chrono::time_point<Clock, Duration>&);

  bool try_lock_until(Deadline deadline);

 private:
  bool LockFastPath(impl::TaskContext*);
  bool LockSlowPath(impl::TaskContext*, Deadline);

  std::atomic<impl::TaskContext*> owner_;
  impl::FastPimplWaitList lock_waiters_;
};

template <typename Rep, typename Period>
bool Mutex::try_lock_for(const std::chrono::duration<Rep, Period>& duration) {
  return try_lock_until(Deadline::FromDuration(duration));
}

template <typename Clock, typename Duration>
bool Mutex::try_lock_until(
    const std::chrono::time_point<Clock, Duration>& until) {
  return try_lock_until(Deadline::FromTimePoint(until));
}

}  // namespace engine
