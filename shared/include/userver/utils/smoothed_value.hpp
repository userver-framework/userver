#pragma once

/// @file userver/utils/smoothed_value.hpp
/// @brief @copybrief utils::SmoothedValue

#include <numeric>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @brief A class for calculating average values from the last N values.
/// It fits for small N values because GetSmoothed() has O(N) complexity.
template <typename T>
class SmoothedValue final {
 public:
  explicit SmoothedValue(std::size_t buckets_count) : buckets_(buckets_count) {}

  void Update(T value) {
    buckets_[idx_] = value;
    idx_ = (idx_ + 1) % buckets_.size();
  }

  T GetSmoothed() const {
    return std::accumulate(buckets_.begin(), buckets_.end(),
                           static_cast<T>(0)) /
           buckets_.size();
  }

 private:
  std::vector<T> buckets_;
  std::size_t idx_{0};
};

}  // namespace utils

USERVER_NAMESPACE_END
