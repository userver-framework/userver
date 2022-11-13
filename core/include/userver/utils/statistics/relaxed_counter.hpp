#pragma once

/// @file userver/utils/statistics/relaxed_counter.hpp
/// @brief @copybrief utils::statistics::RelaxedCounter

#include <atomic>

#include <userver/utils/statistics/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

/// @brief Atomic counter of type T with relaxed memory ordering
template <class T>
class RelaxedCounter final {
 public:
  using ValueType = T;

  constexpr RelaxedCounter() noexcept = default;
  constexpr RelaxedCounter(T desired) noexcept : val_(desired) {}

  RelaxedCounter(const RelaxedCounter& other) noexcept : val_(other.Load()) {}

  RelaxedCounter& operator=(const RelaxedCounter& other) noexcept {
    if (this == &other) return *this;

    Store(other.Load());
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

  T operator++(int) noexcept {
    return val_.fetch_add(1, std::memory_order_relaxed);
  }

  RelaxedCounter& operator--() noexcept {
    val_.fetch_sub(1, std::memory_order_relaxed);
    return *this;
  }

  T operator--(int) noexcept {
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
  static_assert(std::atomic<T>::is_always_lock_free);

  std::atomic<T> val_{T{}};
};

template <typename T>
void DumpMetric(Writer& writer, const RelaxedCounter<T>& value) {
  writer = value.Load();
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
