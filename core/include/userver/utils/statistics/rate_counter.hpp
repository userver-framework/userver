#pragma once

/// @file userver/utils/statistics/rate_counter.hpp
/// @brief @copybrief utils::statistics::RateCounter

#include <atomic>

#include <userver/utils/statistics/fwd.hpp>
#include <userver/utils/statistics/rate.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

/// @brief Atomic counter of type Rate with relaxed memory ordering
///
/// This class is represented as Rate metric when serializing to statistics.
/// Otherwise it is the same class as RelaxedCounter
class RateCounter final {
 public:
  using ValueType = Rate;

  constexpr RateCounter() noexcept = default;
  constexpr explicit RateCounter(Rate desired) noexcept : val_(desired.value) {}
  constexpr explicit RateCounter(Rate::ValueType desired) noexcept
      : val_(desired) {}

  RateCounter(const RateCounter& other) noexcept : val_(other.Load().value) {}

  RateCounter& operator=(const RateCounter& other) noexcept {
    if (this == &other) return *this;

    Store(other.Load());
    return *this;
  }

  RateCounter& operator=(Rate desired) noexcept {
    Store(desired);
    return *this;
  }

  void Store(Rate desired,
             std::memory_order order = std::memory_order_relaxed) noexcept {
    val_.store(desired.value, order);
  }

  Rate Load() const noexcept {
    return Rate{val_.load(std::memory_order_relaxed)};
  }

  void Add(Rate arg,
           std::memory_order order = std::memory_order_relaxed) noexcept {
    val_.fetch_add(arg.value, order);
  }

  RateCounter& operator++() noexcept {
    val_.fetch_add(1, std::memory_order_relaxed);
    return *this;
  }

  Rate operator++(int) noexcept {
    return Rate{val_.fetch_add(1, std::memory_order_relaxed)};
  }

  RateCounter& operator+=(Rate arg) noexcept {
    val_.fetch_add(arg.value, std::memory_order_relaxed);
    return *this;
  }

  RateCounter& operator+=(RateCounter arg) noexcept {
    return *this += arg.Load();
  }

 private:
  static_assert(std::atomic<Rate::ValueType>::is_always_lock_free);

  std::atomic<Rate::ValueType> val_{0};
};

void DumpMetric(Writer& writer, const RateCounter& value);

void ResetMetric(RateCounter& value);

}  // namespace utils::statistics

USERVER_NAMESPACE_END
