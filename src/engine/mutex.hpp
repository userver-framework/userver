#pragma once

#include <memory>
#include <mutex>  // for std locks

#include <engine/wait_list.hpp>

namespace engine {
namespace impl {

class TaskContext;

}  // namespace impl

class Mutex {
 public:
  Mutex();
  ~Mutex();

  Mutex(const Mutex&) = delete;
  Mutex(Mutex&&) = delete;
  Mutex& operator=(const Mutex&) = delete;
  Mutex& operator=(Mutex&&) = delete;

  void lock();
  void unlock();

  // TODO: try_lock, try_lock_for, try_lock_until

 private:
  std::shared_ptr<WaitList> lock_waiters_;
  impl::TaskContext* owner_;
};

}  // namespace engine
