#pragma once

#include <array>
#include <atomic>

#include "recentperiod.hpp"

namespace utils {
namespace statistics {

/** @brief Class stores M buckets of type Counter and allows easy
 * calculation of percentiles.
 *
 * Bucket X stores how many elements with value X was accounted for, for the
 * first M buckets. Bucket X stores how many elements with values
 * [X-ExtraBucketSize/2; X+ExtraBucketSize/2) were accounted for, for the next
 * ExtraBuckets buckets.
 *
 * @tparam M how many buckets store precise value
 * @tparam Counter bucket type
 * @tparam ExtraBuckets how many buckets store approximated values
 * @tparam ExtraBucketSize ExtraBuckets store values with this precision
 * @see GetPercentile
 * @see Account
 */
template <size_t M, typename Counter = unsigned int, size_t ExtraBuckets = 0,
          size_t ExtraBucketSize = 500>
class Percentile {
 public:
  Percentile() {
    for (auto& value : values) value.store(0, std::memory_order_relaxed);
    for (auto& value : extra_values) value.store(0, std::memory_order_relaxed);
    count = 0;
  }

  Percentile(
      const Percentile<M, Counter, ExtraBuckets, ExtraBucketSize>& other) {
    size_t sum = 0;
    for (size_t i = 0; i < values.size(); i++) {
      const auto value = other.values[i].load(std::memory_order_relaxed);
      values[i].store(value, std::memory_order_relaxed);
      sum += value;
    }

    for (size_t i = 0; i < extra_values.size(); i++) {
      const auto value = other.extra_values[i].load(std::memory_order_relaxed);
      extra_values[i].store(value, std::memory_order_relaxed);
      sum += value;
    }
    count = sum;
  }

  /** \brief Account for another value. Value is truncated [0..M) and
   *  added to the corresponding bucket
   */
  void Account(size_t value) {
    value = std::max<size_t>(0, value);
    if (value < values.size()) {
      values[value]++;
    } else {
      if (extra_values.size()) {
        size_t extra_bucket =
            (value - values.size() + ExtraBucketSize / 2) / ExtraBucketSize;
        extra_bucket = std::min<size_t>(extra_bucket, extra_values.size() - 1);
        extra_values[extra_bucket]++;
      } else {
        values.back()++;
      }
    }
    count++;
  }

  /** \brief Get X percentile - min value P in [0..M) so that total number
   * of elements in buckets 0..P is no less than X percent
   * @param percent - value in [0..100] - requested percentile
   *                  if outside of 100, then returns last bucket that
   *                  has any element in it.
   */
  size_t GetPercentile(double percent) const {
    if (count == 0) return 0;

    size_t sum = 0;
    size_t want_sum = count * percent;
    size_t max_value = 0;
    for (size_t i = 0; i < values.size(); i++) {
      const auto value = values[i].load(std::memory_order_relaxed);
      sum += value;
      if (sum * 100 > want_sum) return i;

      if (value) max_value = i;
    }

    for (size_t i = 0; i < extra_values.size(); i++) {
      const auto value = extra_values[i].load(std::memory_order_relaxed);
      sum += value;
      if (sum * 100 > want_sum) return ExtraBucketToValue(i);

      if (value) max_value = ExtraBucketToValue(i);
    }

    return max_value;
  }

  template <class Duration = std::chrono::seconds>
  void Add(const Percentile<M, Counter, ExtraBuckets, ExtraBucketSize>& other,
           Duration this_epoch_duration = Duration(),
           Duration before_this_epoch_duration = Duration()) {
    std::ignore = this_epoch_duration;
    std::ignore = before_this_epoch_duration;

    size_t sum = 0;
    for (size_t i = 0; i < values.size(); i++) {
      const auto value = other.values[i].load(std::memory_order_relaxed);
      sum += value;
      values[i].fetch_add(value, std::memory_order_relaxed);
    }

    for (size_t i = 0; i < extra_values.size(); i++) {
      const auto value = other.extra_values[i].load(std::memory_order_relaxed);
      sum += value;
      extra_values[i].fetch_add(value, std::memory_order_relaxed);
    }
    count += sum;
  }

  void Reset() {
    for (auto& value : values) value.store(0, std::memory_order_relaxed);
    for (auto& value : extra_values) value.store(0, std::memory_order_relaxed);
    count = 0;
  }

  /** \brief Total number of elements
   */
  Counter Count() const { return count; }

 private:
  size_t ExtraBucketToValue(size_t bucket) const {
    return values.size() + bucket * ExtraBucketSize;
  }

 private:
  std::array<std::atomic<Counter>, M> values;
  std::array<std::atomic<Counter>, ExtraBuckets> extra_values;
  std::atomic<Counter> count;
};

}  // namespace statistics
}  // namespace utils
