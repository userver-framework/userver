#pragma once

#include <userver/cache/impl/sketch/bloom.hpp>
#include <userver/cache/impl/sketch/doorkeeper.hpp>
#include "userver/utils/assert.hpp"

USERVER_NAMESPACE_BEGIN

namespace cache::impl::sketch {
template <typename T, typename Hash>
class Sketch<T, Hash, Policy::Aged> {
 public:
  explicit Sketch(size_t num_counters, const Hash& hash = Hash{});
  int Estimate(const T&);
  void Increment(const T&);
  void Clear() { bloom_.Clear(); }

 private:
  Sketch<T, Hash, Policy::Doorkeeper> bloom_;
  size_t sample_size_;
  size_t size_{0};

  static constexpr size_t sample_rate = 10;
};

template <typename T, typename Hash>
Sketch<T, Hash, Policy::Aged>::Sketch(size_t num_counters, const Hash& hash)
    : bloom_(num_counters, hash),
      sample_size_(NextPowerOfTwo(num_counters) * sample_rate) {
  UASSERT(num_counters > 0);
}

template <typename T, typename Hash>
int Sketch<T, Hash, Policy::Aged>::Estimate(const T& item) {
  return bloom_.Estimate(item);
}

template <typename T, typename Hash>
void Sketch<T, Hash, Policy::Aged>::Increment(const T& item) {
  if (bloom_.Increment(item) && (++size_ == sample_size_)) {
    bloom_.Reset();
    size_ /= 2;
  }
}

}  // namespace cache::impl::sketch

USERVER_NAMESPACE_END
