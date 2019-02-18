#pragma once

#include <atomic>
#include <memory>

namespace engine {

namespace impl {
class WaitList;
}

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

  void lock();
  void unlock();

 private:
  std::shared_ptr<impl::WaitList> lock_waiters_;
  std::atomic<std::size_t> remaining_simultaneous_locks_;
};

}  // namespace engine
