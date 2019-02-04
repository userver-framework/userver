#pragma once

#include <atomic>

namespace utils {
namespace statistics {

template <class T>
class RelaxedCounter {
 public:
  using ValueType = T;

 public:
  RelaxedCounter() noexcept = default;
  constexpr RelaxedCounter(T desired) noexcept : val_(std::move(desired)) {}
  RelaxedCounter(RelaxedCounter&& other) noexcept : val_(other.Load()) {}

  RelaxedCounter& operator=(RelaxedCounter&& rhs) noexcept {
    val_ = std::move(rhs.val_);
    return *this;
  }

  RelaxedCounter& operator=(T desired) noexcept {
    Store(desired);
    return *this;
  }

  void Store(T desired) noexcept {
    val_.store(desired, std::memory_order_relaxed);
  }

  T Load() const noexcept { return val_.load(std::memory_order_relaxed); }

  operator T() const noexcept { return Load(); }

  RelaxedCounter& operator++() noexcept {
    val_.fetch_add(1, std::memory_order_relaxed);
    return *this;
  }

  T operator++(int)noexcept {
    return val_.fetch_add(1, std::memory_order_relaxed);
  }

  RelaxedCounter& operator--() noexcept {
    val_.fetch_sub(1, std::memory_order_relaxed);
    return *this;
  }

  T operator--(int)noexcept {
    return val_.fetch_sub(1, std::memory_order_relaxed);
  }

  RelaxedCounter& operator+=(T arg) noexcept {
    val_.fetch_add(arg, std::memory_order_relaxed);
    return *this;
  }

  RelaxedCounter& operator-=(T arg) noexcept {
    val_.fetch_sub(arg, std::memory_order_relaxed);
    return *this;
  }

 private:
  std::atomic<T> val_{T{}};
};

}  // namespace statistics
}  // namespace utils
