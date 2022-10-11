#pragma once

#include <atomic>
#include <cerrno>
#include <condition_variable>
#include <cstdint>
#include <mutex>

#if defined(__unix__)
#include <semaphore.h>
#endif

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

class StdCvSemaphore final {
 public:
  explicit StdCvSemaphore(std::int32_t initial_count) noexcept
      : count_(initial_count) {
    UASSERT(initial_count >= 0);
  }

  void WaitBlocking() noexcept {
    std::unique_lock lock(mutex_);
    const auto new_count = count_.load(std::memory_order_relaxed) - 1;
    count_.store(new_count, std::memory_order_relaxed);
    if (new_count >= 0) return;

    cv_.wait(lock, [&] { return count_.load(std::memory_order_relaxed) >= 0; });
  }

  void Post() noexcept {
    std::unique_lock lock(mutex_);
    const auto old_count = count_.load(std::memory_order_relaxed);
    count_.store(old_count + 1, std::memory_order_relaxed);

    lock.unlock();
    if (old_count < 0) {
      cv_.notify_one();
    }
  }

  // - if >= 0, returns the number of tokens present
  // - if < 0, returns the number of threads waiting
  std::int32_t GetCountApprox() const noexcept {
    return count_.load(std::memory_order_relaxed);
  }

 private:
  std::mutex mutex_;
  std::condition_variable cv_;
  std::atomic<std::int32_t> count_;
};

#if defined(__unix__)
// Taken from Moodycamel Semaphore
class PosixSemaphore final {
 public:
  PosixSemaphore(std::int32_t initial_count = 0) noexcept {
    UASSERT(initial_count >= 0);
    const int rc =
        sem_init(&m_sema, 0, static_cast<unsigned int>(initial_count));
    UASSERT(rc == 0);
  }

  ~PosixSemaphore() { sem_destroy(&m_sema); }

  void WaitBlocking() noexcept {
    if (TryWait()) return;
    int rc{};
    ++waiters_;
    do {
      rc = sem_wait(&m_sema);
    } while (rc == -1 && errno == EINTR);
    --waiters_;
    UASSERT(rc == 0);
  }

  bool TryWait() noexcept {
    int rc{};
    do {
      rc = sem_trywait(&m_sema);
    } while (rc == -1 && errno == EINTR);
    return rc == 0;
  }

  void Post() noexcept {
    while (sem_post(&m_sema) == -1) {
    }
  }

  std::int32_t GetCountApprox() const noexcept {
    int value{};
    const int rc = sem_getvalue(&m_sema, &value);
    UASSERT(rc == 0);
    const auto waiters = waiters_.load();
    return value > 0 ? value : -waiters;
  }

 private:
  mutable sem_t m_sema;
  std::atomic<std::int32_t> waiters_{0};
};

using Semaphore = PosixSemaphore;
#else
using Semaphore = StdCvSemaphore;
#endif

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
