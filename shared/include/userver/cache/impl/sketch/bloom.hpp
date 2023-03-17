#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <climits>

#include <boost/container/small_vector.hpp>

#include <userver/cache/impl/sketch/policy.hpp>
#include <userver/cache/impl/sketch/tools.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache::impl::sketch {

enum class CounterSize {
  kHalfByte = 4,
  kByte = 8
};

constexpr std::size_t kInitialCounters = 32;

// TODO: pass hashed value
template <typename T, typename Hash>
class Sketch<T, Hash, Policy::Bloom> {
 public:
  explicit Sketch(std::size_t num_counters = kInitialCounters, const Hash& hash = Hash{});
  std::size_t Estimate(const T&);
  bool Increment(const T&);
  void Reset();
  void Clear();

 private:
  static constexpr CounterSize kCounterBitSize = CounterSize::kHalfByte;
  static constexpr unsigned char kMaxFrequency = (1u << static_cast<int>(kCounterBitSize)) - 1;
  static constexpr std::size_t kDataBytesCount = (kInitialCounters >> (static_cast<int>(CounterSize::kByte) / static_cast<int>(kCounterBitSize)));
  static constexpr std::size_t kNumHashers = 4;
  static constexpr unsigned char kResetMask = (kCounterBitSize == CounterSize::kByte ? 0x7f : 0x77);

  uint64_t GetHash(const T& item, uint64_t seed);
  unsigned char Get(uint64_t hashed);
  bool TryIncrement(uint64_t hashed);

  std::size_t num_counters_;
  boost::container::small_vector<unsigned char, kDataBytesCount> data_;
  Hash hash_;

  // TODO: random seeds
  static constexpr uint64_t seeds[] = {0xc3a5c85c97cb3127L, 0xb492b66fbe98f273L,
                                       0x9ae16a3b2f90404fL,
                                       0xcbf29ce484222325L};

};

template <typename T, typename Hash>
Sketch<T, Hash, Policy::Bloom>::Sketch(size_t num_counters, const Hash& hash)
    : num_counters_(NextPowerOfTwo(num_counters)),
      data_(num_counters_ >> 1),
      hash_(hash) {
  UASSERT(num_counters > 0);
}

template <typename T, typename Hash>
std::size_t Sketch<T, Hash, Policy::Bloom>::Estimate(const T& item) {
  unsigned char result = std::numeric_limits<unsigned char>::max();
  for (uint64_t seed : seeds) {
    result = std::min(result, Get(GetHash(item, seed)));
  }
  return static_cast<std::size_t>(result);
}

template <typename T, typename Hash>
bool Sketch<T, Hash, Policy::Bloom>::Increment(const T& item) {
  auto was_added = false;
  for (uint64_t seed : seeds) {
    was_added |= TryIncrement(GetHash(item, seed));
  }
  return was_added;
}

template <typename T, typename Hash>
unsigned char Sketch<T, Hash, Policy::Bloom>::Get(uint64_t hashed) {
  std::size_t counter_index = hashed % data_.size();
  if constexpr (kCounterBitSize == CounterSize::kByte) {
    return data_[counter_index];
  } else {
    unsigned char counters_block_value = data_[counter_index >> 1];
    if ((counter_index & 1) == 0) {
      counters_block_value >>= static_cast<int>(CounterSize::kHalfByte);
    }
    return counters_block_value & 0x0f;
  }
}

template <typename T, typename Hash>
uint64_t Sketch<T, Hash, Policy::Bloom>::GetHash(const T& item, uint64_t seed) {
  uint64_t hashed = hash_(item) ^ seed;
  hashed += hashed >> 32;
  return hashed;
}

template <typename T, typename Hash>
bool Sketch<T, Hash, Policy::Bloom>::TryIncrement(uint64_t hashed) {
  std::size_t counter_index = hashed % data_.size();
  if (Get(hashed) == kMaxFrequency) {
    return false;
  }
  if constexpr (kCounterBitSize == CounterSize::kByte) {
    data_[counter_index]++;
  } else {
    data_[counter_index >> 1] += (counter_index & 1) == 0 ? (1u << static_cast<int>(CounterSize::kHalfByte)) : 1;
  }
  return true;
}

template <typename T, typename Hash>
void Sketch<T, Hash, Policy::Bloom>::Reset() {
  for (auto& counter : data_) {
    counter = ((counter >> 1) & kResetMask);
  }
}

template <typename T, typename Hash>
void Sketch<T, Hash, Policy::Bloom>::Clear() {
  for (auto& counter : data_) {
    counter = 0;
  }
}

}  // namespace cache::impl::sketch

USERVER_NAMESPACE_END
