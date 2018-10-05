#pragma once

#include <atomic>
#include <cassert>
#include <chrono>
#include <thread>

namespace engine {
namespace impl {

class TaskCounter {
 public:
  class Token {
   public:
    explicit Token(TaskCounter& counter) : counter_(counter) {
      ++counter_.value_;
    }
    ~Token() { --counter_.value_; }

    Token(const Token&) = delete;
    Token(Token&&) = delete;
    Token& operator=(const Token&) = delete;
    Token& operator=(Token&&) = delete;

   private:
    TaskCounter& counter_;
  };

  TaskCounter() : value_(0) {}
  ~TaskCounter() { assert(!value_); }

  template <typename Rep, typename Period>
  void WaitForExhaustion(
      const std::chrono::duration<Rep, Period>& check_period) const {
    while (value_ > 0) {
      std::this_thread::sleep_for(check_period);
    }
  }

 private:
  std::atomic<size_t> value_;
};

}  // namespace impl
}  // namespace engine
