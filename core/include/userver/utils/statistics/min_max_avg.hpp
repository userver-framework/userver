#pragma once

/// @file userver/utils/statistics/min_max_avg.hpp
/// @brief @copybrief utils::statistics::MinMaxAvg

#include <atomic>
#include <chrono>
#include <cstddef>
#include <type_traits>

#include <userver/formats/json/inline.hpp>
#include <userver/formats/serialize/to.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

/// @brief Class for concurrent safe calculation of minimum, maximum and
/// average over series of values.
template <typename ValueType, typename AverageType = ValueType>
class MinMaxAvg final {
  static_assert(std::is_integral_v<ValueType> &&
                    !std::is_same_v<ValueType, bool>,
                "only integral value types are supported in MinMaxAvg");
  static_assert(std::is_same_v<AverageType, ValueType> ||
                    std::is_floating_point_v<AverageType>,
                "MinMaxAvg average type must either be equal to value type or "
                "be a floating point type");
  static_assert(std::atomic<ValueType>::is_always_lock_free &&
                    std::atomic<size_t>::is_always_lock_free,
                "refusing to use locking atomics");

 public:
  struct Current {
    ValueType minimum;
    ValueType maximum;
    AverageType average;
  };

  constexpr MinMaxAvg() noexcept
      : minimum_(0), maximum_(0), sum_(0), count_(0) {}

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
  MinMaxAvg(const MinMaxAvg& other) noexcept { *this = other; }

  MinMaxAvg& operator=(const MinMaxAvg& rhs) noexcept {
    if (this == &rhs) return *this;

    const auto count = rhs.count_.load(std::memory_order_acquire);
    minimum_ = rhs.minimum_.load(std::memory_order_relaxed);
    maximum_ = rhs.maximum_.load(std::memory_order_relaxed);
    sum_ = rhs.sum_.load(std::memory_order_relaxed);
    count_ = count;
    return *this;
  }

  Current GetCurrent() const {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    Current current;
    const auto count = count_.load(std::memory_order_acquire);
    UASSERT(count >= 0);
    current.minimum = minimum_.load(std::memory_order_relaxed);
    current.maximum = maximum_.load(std::memory_order_relaxed);
    current.average =
        count ? static_cast<AverageType>(sum_.load(std::memory_order_relaxed)) /
                    static_cast<AverageType>(count)
              : AverageType{0};
    return current;
  }

  void Account(ValueType value) {
    ValueType current_minimum = minimum_.load(std::memory_order_relaxed);
    while (current_minimum > value || !count_.load(std::memory_order_relaxed)) {
      if (minimum_.compare_exchange_weak(current_minimum, value,
                                         std::memory_order_relaxed)) {
        break;
      }
    }
    ValueType current_maximum = maximum_.load(std::memory_order_relaxed);
    while (current_maximum < value || !count_.load(std::memory_order_relaxed)) {
      if (maximum_.compare_exchange_weak(current_maximum, value,
                                         std::memory_order_relaxed)) {
        break;
      }
    }
    sum_.fetch_add(value, std::memory_order_relaxed);
    count_.fetch_add(1, std::memory_order_release);
  }

  template <class Duration = std::chrono::seconds>
  void Add(const MinMaxAvg& other,
           [[maybe_unused]] Duration this_epoch_duration = Duration(),
           [[maybe_unused]] Duration before_this_epoch_duration = Duration()) {
    ValueType current_minimum = minimum_.load(std::memory_order_relaxed);
    while (current_minimum > other.minimum_.load(std::memory_order_relaxed) ||
           !count_.load(std::memory_order_relaxed)) {
      if (minimum_.compare_exchange_weak(
              current_minimum, other.minimum_.load(std::memory_order_relaxed),
              std::memory_order_relaxed)) {
        break;
      }
    }
    ValueType current_maximum = maximum_.load(std::memory_order_relaxed);
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

  bool IsEmpty() const noexcept { return count_.load() == 0; }

 private:
  std::atomic<ValueType> minimum_;
  std::atomic<ValueType> maximum_;
  std::atomic<ValueType> sum_;
  std::atomic<ssize_t> count_;
};

template <typename ValueType, typename AverageType>
auto Serialize(const MinMaxAvg<ValueType, AverageType>& mma,
               formats::serialize::To<formats::json::Value>) {
  const auto current = mma.GetCurrent();
  return formats::json::MakeObject("min", current.minimum, "max",
                                   current.maximum, "avg", current.average);
}

template <typename ValueType, typename AverageType>
void DumpMetric(Writer& writer,
                const MinMaxAvg<ValueType, AverageType>& value) {
  const auto current = value.GetCurrent();
  writer["min"] = current.minimum;
  writer["max"] = current.maximum;
  writer["avg"] = current.average;
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
