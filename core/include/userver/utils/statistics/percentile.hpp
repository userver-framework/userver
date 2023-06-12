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
 *
 * Example:
 * Precisely count for first 500 milliseconds of execution using uint32_t
 * counters and leave 100 buckets for all the other values with precision
 * of 42 milliseconds:
 *
 * @code
 * using Percentile =
 *   utils::statistics::Percentile<500, std::uint32_t, 100, 42>;
 *
 * void Account(Percentile& perc, std::chrono::milliseconds ms) {
 *   perc.Account(ms.count());
 * }
 *
 * formats::json::Value ExtendStatistics(
 *   formats::json::ValueBuilder& stats_builder, Percentile& perc) {
 *
 *   stats_builder["timings"]["1min"] = PercentileToJson(perc).ExtractValue();
 *
 *   return stats_builder.ExtractValue();
 * }
 * @endcode
 */
template <size_t M, typename Counter = uint32_t, size_t ExtraBuckets = 0,
          size_t ExtraBucketSize = 500>
class Percentile final {
 public:
  Percentile() {
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

    size_t sum = 0;
    for (size_t i = 0; i < values_.size(); i++) {
      const auto value = rhs.values_[i].load(std::memory_order_relaxed);
      values_[i].store(value, std::memory_order_relaxed);
      sum += value;
    }

    for (size_t i = 0; i < extra_values_.size(); i++) {
      const auto value = rhs.extra_values_[i].load(std::memory_order_relaxed);
      extra_values_[i].store(value, std::memory_order_relaxed);
      sum += value;
    }
    count_ = sum;
    return *this;
  }

  /** \brief Account for another value. Value is truncated [0..M) and
   *  added to the corresponding bucket
   */
  void Account(size_t value) {
    if (value < values_.size()) {
      values_[value].fetch_add(1, std::memory_order_relaxed);
    } else {
      if (!extra_values_.empty()) {
        size_t extra_bucket =
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

  /** \brief Get X percentile - min value P in [0..M) so that total number
   * of elements in buckets 0..P is no less than X percent
   * @param percent - value in [0..100] - requested percentile
   *                  if outside of 100, then returns last bucket that
   *                  has any element in it.
   */
  size_t GetPercentile(double percent) const {
    if (count_ == 0) return 0;

    size_t sum = 0;
    size_t want_sum = count_.load(std::memory_order_acquire) * percent;
    size_t max_value = 0;
    for (size_t i = 0; i < values_.size(); i++) {
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

  void Reset() {
    for (auto& value : values_) value.store(0, std::memory_order_relaxed);
    for (auto& value : extra_values_) value.store(0, std::memory_order_relaxed);
    count_ = 0;
  }

  /** \brief Total number of elements
   */
  Counter Count() const { return count_; }

 private:
  size_t ExtraBucketToValue(size_t bucket) const {
    return values_.size() + bucket * ExtraBucketSize;
  }

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
