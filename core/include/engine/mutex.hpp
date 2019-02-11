#pragma once

/// @file engine/mutex.hpp
/// @brief @copybrief engine::Mutex

#include <atomic>
#include <memory>
#include <mutex>  // for std locks

namespace engine {
namespace impl {

class TaskContext;
class WaitList;

}  // namespace impl

/// std::mutex replacement for asynchronous tasks
class Mutex {
 public:
  Mutex();
  ~Mutex();

  Mutex(const Mutex&) = delete;
  Mutex(Mutex&&) = delete;
  Mutex& operator=(const Mutex&) = delete;
  Mutex& operator=(Mutex&&) = delete;

  /// Suspends execution until the mutex lock is acquired
  void lock();

  /// Releases mutex lock
  void unlock();

  // TODO: try_lock, try_lock_for, try_lock_until

 private:
  void LockSlowPath(impl::TaskContext* current);

  std::shared_ptr<impl::WaitList> lock_waiters_;
  std::atomic<impl::TaskContext*> owner_;
};

}  // namespace engine
