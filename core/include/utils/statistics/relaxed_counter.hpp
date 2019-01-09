#pragma once

#include <atomic>

namespace utils {
namespace statistics {

template <class T>
class RelaxedCounter {
 public:
  RelaxedCounter() noexcept = default;
  constexpr RelaxedCounter(T desired) noexcept : val_(std::move(desired)) {}

  T operator=(T desired) noexcept {
    Store(desired);
    return desired;
  }

  void Store(T desired) noexcept {
    val_.store(desired, std::memory_order_relaxed);
  }

  T Load() const noexcept { return val_.load(std::memory_order_relaxed); }

  operator T() const noexcept { return Load(); }

  T operator++() noexcept {
    return val_.fetch_add(1, std::memory_order_relaxed) + 1;
  }

  T operator++(int)noexcept {
    return val_.fetch_add(1, std::memory_order_relaxed);
  }

  T operator--() noexcept {
    return val_.fetch_sub(1, std::memory_order_relaxed) - 1;
  }

  T operator--(int)noexcept {
    return val_.fetch_sub(1, std::memory_order_relaxed);
  }

  T operator+=(T arg) noexcept {
    return val_.fetch_add(arg, std::memory_order_relaxed) + arg;
  }

  T operator-=(T arg) noexcept {
    return val_.fetch_sub(arg, std::memory_order_relaxed) - arg;
  }

 private:
  std::atomic<T> val_{T{}};
};

}  // namespace statistics
}  // namespace utils
