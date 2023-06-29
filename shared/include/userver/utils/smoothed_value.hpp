#pragma once

/// @file userver/utils/smoothed_value.hpp
/// @brief @copybrief utils::SmoothedValue

#include <numeric>
#include <vector>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

template <typename T>
class AggregatedValue {
 public:
  explicit AggregatedValue(std::size_t buckets_count)
      : buckets_(buckets_count) {}

  void Update(T value) {
    buckets_[idx_] = value;
    idx_ = (idx_ + 1) % buckets_.size();
  }

 protected:
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  std::vector<T> buckets_;
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  std::size_t idx_{0};
};

/// @brief A class for calculating average values from the last N values.
/// It fits for small N values because GetSmoothed() has O(N) complexity.
template <typename T>
class SmoothedValue final : public AggregatedValue<T> {
 public:
  using AggregatedValue<T>::AggregatedValue;

  T GetSmoothed() const {
    UASSERT(this->buckets_.size() > 0);
    return std::accumulate(this->buckets_.begin(), this->buckets_.end(),
                           static_cast<T>(0)) /
           this->buckets_.size();
  }
};

/// @brief A class for calculating min value from the last N values.
/// It fits for small N values because GetMinimal() has O(N) complexity.
template <typename T>
class MinimalValue final : public AggregatedValue<T> {
 public:
  using AggregatedValue<T>::AggregatedValue;

  T GetMinimal() const {
    UASSERT(this->buckets_.size() > 0);
    return *std::min_element(this->buckets_.begin(), this->buckets_.end());
  }
};

}  // namespace utils

USERVER_NAMESPACE_END
