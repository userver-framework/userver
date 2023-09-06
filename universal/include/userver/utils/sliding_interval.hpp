#pragma once

/// @file userver/utils/sliding_interval.hpp
/// @brief @copybrief utils::SlidingInterval

#include <numeric>

#include <userver/utils/assert.hpp>
#include <userver/utils/fixed_array.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @ingroup userver_universal userver_containers
///
/// @brief Sliding interval of values that provides functions to compute
/// average, min and max values from the last `window_size` values of interval.
///
/// It fits for small `window_size` values because has O(window_size) complexity
/// on most operations.
///
/// @see utils::statistics::MinMaxAvg for a concurrent safe computation over
/// a whole measurement interval.
template <typename T>
class SlidingInterval final {
 public:
  /// @brief Create a SlidingInterval of `window_size` values
  explicit SlidingInterval(std::size_t window_size) : buckets_(window_size) {
    UASSERT(this->buckets_.size() > 0);
  }

  /// @brief replaces the oldest value in interval with `value`, i.e slides the
  /// interval.
  void Update(T value) {
    buckets_[idx_] = value;
    ++idx_;
    if (idx_ == buckets_.size()) {
      idx_ = 0;
    }
  }

  /// @returns Average value in the interval
  ///
  /// \b Complexity: O(window_size)
  [[nodiscard]] T GetSmoothed() const {
    return std::accumulate(buckets_.begin(), buckets_.end(),
                           static_cast<T>(0)) /
           this->buckets_.size();
  }

  /// @returns Minimal value in the interval
  ///
  /// \b Complexity: O(window_size)
  [[nodiscard]] T GetMinimal() const {
    return *std::min_element(buckets_.begin(), buckets_.end());
  }

  /// @returns Maximum value in the interval
  ///
  /// \b Complexity: O(window_size)
  [[nodiscard]] T GetMaximum() const {
    return *std::max_element(buckets_.begin(), buckets_.end());
  }

  /// @returns Elements count in the interval, i.e. `window_size` passed to
  /// constructor.
  [[nodiscard]] std::size_t GetWindowSize() const { return buckets_.size(); }

 private:
  utils::FixedArray<T> buckets_;
  std::size_t idx_{0};
};

}  // namespace utils

USERVER_NAMESPACE_END
