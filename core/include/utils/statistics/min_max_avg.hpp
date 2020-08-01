#pragma once

#include <atomic>
#include <cstddef>
#include <type_traits>

#include <formats/json/inline.hpp>
#include <formats/serialize/to.hpp>

/// @file utils/statistics/min_max_avg.hpp
/// @brief @copybrief utils::statistics::MinMaxAvg

namespace utils::statistics {

/// Class calculating minimum, maximum and average over series of values
template <typename T>
class MinMaxAvg final {
  static_assert(std::is_integral_v<T> && !std::is_same_v<T, bool>,
                "only integral types are supported in MinMaxAvg");
  static_assert(std::atomic<T>::is_always_lock_free &&
                    std::atomic<size_t>::is_always_lock_free,
                "refusing to use locking atomics");

 public:
  struct Current {
    T minimum;
    T maximum;
    T average;
  };

  MinMaxAvg() noexcept : minimum_(0), maximum_(0), sum_(0), count_(0) {}

  MinMaxAvg(MinMaxAvg&& other) noexcept
      : minimum_(other.minimum_.load(std::memory_order_relaxed)),
        maximum_(other.maximum_.load(std::memory_order_relaxed)),
        sum_(other.sum_.load(std::memory_order_relaxed)),
        count_(other.count_.load(std::memory_order_acquire)) {}

  Current GetCurrent() const {
    Current current;
    const auto count = count_.load(std::memory_order_acquire);
    current.minimum = minimum_.load(std::memory_order_relaxed);
    current.maximum = maximum_.load(std::memory_order_relaxed);
    current.average = count ? sum_.load(std::memory_order_relaxed) / count : 0;
    return current;
  }

  void Account(T value) {
    T current_minimum = minimum_.load(std::memory_order_relaxed);
    while (current_minimum > value || !count_.load(std::memory_order_relaxed)) {
      if (minimum_.compare_exchange_weak(current_minimum, value,
                                         std::memory_order_relaxed)) {
        break;
      }
    }
    T current_maximum = maximum_.load(std::memory_order_relaxed);
    while (current_maximum < value || !count_.load(std::memory_order_relaxed)) {
      if (maximum_.compare_exchange_weak(current_maximum, value,
                                         std::memory_order_relaxed)) {
        break;
      }
    }
    sum_.fetch_add(value, std::memory_order_relaxed);
    count_.fetch_add(1, std::memory_order_release);
  }

  void Add(const MinMaxAvg& other) {
    T current_minimum = minimum_.load(std::memory_order_relaxed);
    while (current_minimum > other.minimum_.load(std::memory_order_relaxed) ||
           !count_.load(std::memory_order_relaxed)) {
      if (minimum_.compare_exchange_weak(
              current_minimum, other.minimum_.load(std::memory_order_relaxed),
              std::memory_order_relaxed)) {
        break;
      }
    }
    T current_maximum = maximum_.load(std::memory_order_relaxed);
    while (current_maximum < other.maximum_.load(std::memory_order_relaxed) ||
           !count_.load(std::memory_order_relaxed)) {
      if (maximum_.compare_exchange_weak(
              current_maximum, other.maximum_.load(std::memory_order_relaxed),
              std::memory_order_relaxed)) {
        break;
      }
    }
    sum_.fetch_add(other.sum_.load(std::memory_order_relaxed),
                   std::memory_order_relaxed);
    count_.fetch_add(other.count_.load(std::memory_order_acquire),
                     std::memory_order_release);
  }

  void Reset() {
    minimum_.store(0, std::memory_order_relaxed);
    maximum_.store(0, std::memory_order_relaxed);
    sum_.store(0, std::memory_order_relaxed);
    count_ = 0;
  }

 private:
  std::atomic<T> minimum_;
  std::atomic<T> maximum_;
  std::atomic<T> sum_;
  std::atomic<size_t> count_;
};

template <typename T>
auto Serialize(const MinMaxAvg<T>& mma,
               formats::serialize::To<formats::json::Value>) {
  const auto current = mma.GetCurrent();
  return formats::json::MakeObject("min", current.minimum, "max",
                                   current.maximum, "avg", current.average);
}

}  // namespace utils::statistics
