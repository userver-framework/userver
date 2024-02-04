#pragma once

/// @file userver/utils/statistics/percentile.hpp
/// @brief @copybrief utils::statistics::Percentile

#include <array>
#include <atomic>
#include <chrono>
#include <string>

#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

/** @brief Class allows easy calculation of percentiles.
 *
 * Stores `M + ExtraBuckets` buckets of type `Counter`.
 *
 * On `Account(x)` the bucket to increment is chosen depending on `x` value.
 * If `x` belongs to:
 *
 * - `[0, M)`, then bucket from `M` buckets;
 * - `[M, M + ExtraBuckets * ExtraBucketSize)`, then bucket from `ExtraBuckets`;
 * - all the bigger values, then last bucket from `ExtraBuckets`.
 *
 * On `GetPercentile(percent)` class sums up the required amount of buckets,
 * knowing the total count of `Account(x)` invocations.
 *
 * @tparam M buckets count for value step of `1`
 * @tparam Counter type of all the buckets
 * @tparam ExtraBuckets buckets for value step of `ExtraBucketSize`
 * @tparam ExtraBucketSize `ExtraBuckets` store values with this precision
 * @see GetPercentile
 * @see Account
 *
 * @b Example:
 * Precisely count for first 500 milliseconds of execution using `std::uint32_t`
 * counters and leave 100 buckets for all the other values with precision
 * of 9 milliseconds:
 *
 * @code
 * using Percentile = utils::statistics::Percentile<500, std::uint32_t, 100, 9>;
 * using Timings = utils::statistics::RecentPeriod<Percentile, Percentile>;
 *
 * void Account(Percentile& perc, std::chrono::milliseconds ms) {
 *   perc.Account(ms.count());
 * }
 *
 * void ExtendStatistics(utils::statistics::Writer& writer, Timings& timings) {
 *   writer["timings"]["1min"] = timings;
 * }
 * @endcode
 *
 * Type is safe to read/write concurrently from different threads/coroutines.
 *
 * `Percentile` metrics are non-summable, e.g. given RPS counts and timing
 * percentiles for multiple hosts or handlers, we cannot accurately compute the
 * total timing percentiles.
 *
 * @see utils::statistics::Histogram for the summable equivalent
 */
template <std::size_t M, typename Counter = std::uint32_t,
          std::size_t ExtraBuckets = 0, std::size_t ExtraBucketSize = 500>
class Percentile final {
 public:
  Percentile() noexcept {
    for (auto& value : values_) value.store(0, std::memory_order_relaxed);
    for (auto& value : extra_values_) value.store(0, std::memory_order_relaxed);
    count_.store(0, std::memory_order_release);
  }

  Percentile(const Percentile<M, Counter, ExtraBuckets, ExtraBucketSize>&
                 other) noexcept {
    *this = other;
  }

  Percentile& operator=(const Percentile& rhs) noexcept {
    if (this == &rhs) return *this;

    std::size_t sum = 0;
    for (std::size_t i = 0; i < values_.size(); i++) {
      const auto value = rhs.values_[i].load(std::memory_order_relaxed);
      values_[i].store(value, std::memory_order_relaxed);
      sum += value;
    }

    for (std::size_t i = 0; i < extra_values_.size(); i++) {
      const auto value = rhs.extra_values_[i].load(std::memory_order_relaxed);
      extra_values_[i].store(value, std::memory_order_relaxed);
      sum += value;
    }

    count_ = sum;
    return *this;
  }

  /// @brief Account for another value.
  ///
  /// `1` is added to the bucket corresponding to `value`
  void Account(std::size_t value) noexcept {
    if (value < values_.size()) {
      values_[value].fetch_add(1, std::memory_order_relaxed);
    } else {
      if (!extra_values_.empty()) {
        std::size_t extra_bucket =
            (value - values_.size() + ExtraBucketSize / 2) / ExtraBucketSize;
        if (extra_bucket >= extra_values_.size()) {
          extra_bucket = extra_values_.size() - 1;
        }
        extra_values_[extra_bucket].fetch_add(1, std::memory_order_relaxed);
      } else {
        values_.back().fetch_add(1, std::memory_order_relaxed);
      }
    }
    count_.fetch_add(1, std::memory_order_release);
  }

  /// @brief Get X percentile - min value P so that total number
  /// of elements in buckets is no less than X percent.
  ///
  /// @param percent - value in [0..100] - requested percentile.
  /// If outside of 100, then returns last bucket that has any element in it.
  std::size_t GetPercentile(double percent) const {
    if (count_ == 0) return 0;

    std::size_t sum = 0;
    std::size_t want_sum = count_.load(std::memory_order_acquire) * percent;
    std::size_t max_value = 0;
    for (std::size_t i = 0; i < values_.size(); i++) {
      const auto value = values_[i].load(std::memory_order_relaxed);
      sum += value;
      if (sum * 100 > want_sum) return i;

      if (value) max_value = i;
    }

    for (size_t i = 0; i < extra_values_.size(); i++) {
      const auto value = extra_values_[i].load(std::memory_order_relaxed);
      sum += value;
      if (sum * 100 > want_sum) return ExtraBucketToValue(i);

      if (value) max_value = ExtraBucketToValue(i);
    }

    return max_value;
  }

  template <class Duration = std::chrono::seconds>
  void Add(const Percentile<M, Counter, ExtraBuckets, ExtraBucketSize>& other,
           [[maybe_unused]] Duration this_epoch_duration = Duration(),
           [[maybe_unused]] Duration before_this_epoch_duration = Duration()) {
    size_t sum = 0;
    for (size_t i = 0; i < values_.size(); i++) {
      const auto value = other.values_[i].load(std::memory_order_relaxed);
      sum += value;
      values_[i].fetch_add(value, std::memory_order_relaxed);
    }

    for (size_t i = 0; i < extra_values_.size(); i++) {
      const auto value = other.extra_values_[i].load(std::memory_order_relaxed);
      sum += value;
      extra_values_[i].fetch_add(value, std::memory_order_relaxed);
    }
    count_.fetch_add(sum, std::memory_order_release);
  }

  /// @brief Zero out all the buckets and total number of elements.
  void Reset() noexcept {
    for (auto& value : values_) value.store(0, std::memory_order_relaxed);
    for (auto& value : extra_values_) value.store(0, std::memory_order_relaxed);
    count_ = 0;
  }

  /// @brief Total number of elements
  Counter Count() const noexcept { return count_; }

 private:
  size_t ExtraBucketToValue(std::size_t bucket) const noexcept {
    return values_.size() + bucket * ExtraBucketSize;
  }

  static_assert(std::atomic<Counter>::is_always_lock_free,
                "`std::atomic<Counter>` is not lock-free. Please choose some "
                "other `Counter` type");

  std::array<std::atomic<Counter>, M> values_;
  std::array<std::atomic<Counter>, ExtraBuckets> extra_values_;
  std::atomic<Counter> count_;
};

std::string GetPercentileFieldName(double perc);

template <size_t M, typename Counter, size_t ExtraBuckets,
          size_t ExtraBucketSize>
void DumpMetric(
    Writer& writer,
    const Percentile<M, Counter, ExtraBuckets, ExtraBucketSize>& perc,
    std::initializer_list<double> percents = {0, 50, 90, 95, 98, 99, 99.6, 99.9,
                                              100}) {
  for (double percent : percents) {
    writer.ValueWithLabels(
        perc.GetPercentile(percent),
        {"percentile", statistics::GetPercentileFieldName(percent)});
  }
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
