#pragma once

#if (__cplusplus >= 202002L)  // C++20
#include <version>
#endif

#if defined(__cpp_lib_latch)

#include <latch>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {
using Latch = std::latch;
}  // namespace concurrent::impl

USERVER_NAMESPACE_END

#else

#include <condition_variable>
#include <mutex>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

class Latch final {
 public:
  inline explicit Latch(std::ptrdiff_t expected) : expected_(expected) {}

  inline void count_down() {
    std::unique_lock lk{mtx_};
    if (expected_ > 0) {
      --expected_;
      if (expected_ == 0) {
        lk.unlock();
        cv_.notify_all();
      }
    }
  }

  inline void wait() {
    std::unique_lock lk{mtx_};
    cv_.wait(lk, [=] { return expected_ == 0; });
  }

 private:
  std::ptrdiff_t expected_{0};
  std::mutex mtx_;
  std::condition_variable cv_;
};

}  // namespace concurrent::impl

USERVER_NAMESPACE_END

#endif
