#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>

#include <userver/cache/impl/sketch/policy.hpp>
#include <userver/cache/impl/sketch/tools.hpp>
#include "userver/utils/assert.hpp"

USERVER_NAMESPACE_BEGIN

namespace cache::impl::sketch {
// TODO: pass hashed value
template <typename T, typename Hash>
class Sketch<T, Hash, Policy::Bloom> {
 public:
  explicit Sketch(size_t num_counters, const Hash& hash = Hash{});
  int Estimate(const T&);
  // int Estimate(uint64_t hashed);
  bool Increment(const T&);
  // bool Increment(uint64_t hashed);
  void Reset();
  void Clear();

 private:
  std::vector<unsigned char> data_;
  size_t num_counters_;
  Hash hash_;
  int size_{0};
  size_t mask_;

  static constexpr size_t counter_size = 4;
  static constexpr size_t num_hashes = 4;
  // TODO: random seeds
  static constexpr uint64_t seeds[] = {0xc3a5c85c97cb3127L, 0xb492b66fbe98f273L,
                                       0x9ae16a3b2f90404fL,
                                       0xcbf29ce484222325L};

  uint64_t GetHash(const T& item, int i);
  size_t GetIndex(size_t i);
  size_t GetOffset(size_t i);
  int Get(uint64_t hashed, int step);

  bool TryIncrement(uint64_t hashed, int step);
};

template <typename T, typename Hash>
Sketch<T, Hash, Policy::Bloom>::Sketch(size_t num_counters, const Hash& hash)
    : data_(NextPowerOfTwo(num_counters) << 1),
      num_counters_(NextPowerOfTwo(num_counters)),
      hash_(hash),
      mask_(NextPowerOfTwo(num_counters) - 1) {
  UASSERT(num_counters > 0);
}

template <typename T, typename Hash>
int Sketch<T, Hash, Policy::Bloom>::Estimate(const T& item) {
  int result = std::numeric_limits<int>::max();
  auto hashed = hash_(item);
  for (int i = 0; i < static_cast<int>(num_hashes); i++)
    result = std::min(result, Get((hashed ^ (hashed >> 1)) ^ seeds[i], i));
  return result;
}

template <typename T, typename Hash>
int Sketch<T, Hash, Policy::Bloom>::Estimate(const T& item) {
  int result = std::numeric_limits<int>::max();
  auto hashed = hash_(item);
  for (int i = 0; i < static_cast<int>(num_hashes); i++)
    result = std::min(result, Get((hashed ^ (hashed >> 1)) ^ seeds[i], i));
  return result;
}

template <typename T, typename Hash>
int Sketch<T, Hash, Policy::Bloom>::Get(uint64_t hashed, int step) {
  auto i = num_counters_ * step + (hashed & mask_);
  return (data_[GetIndex(i)] >> GetOffset(i)) & ((1 << counter_size) - 1);
}

template <typename T, typename Hash>
size_t Sketch<T, Hash, Policy::Bloom>::GetIndex(size_t i) {
  return i >> 1;
}

template <typename T, typename Hash>
size_t Sketch<T, Hash, Policy::Bloom>::GetOffset(size_t i) {
  return (i & 1) << 2;
}

template <typename T, typename Hash>
uint64_t Sketch<T, Hash, Policy::Bloom>::GetHash(const T& item, int i) {
  auto hashed = hash_(item) ^ seeds[i];
  hashed += hashed >> 32;
  return hashed;
}

template <typename T, typename Hash>
bool Sketch<T, Hash, Policy::Bloom>::TryIncrement(uint64_t hashed, int step) {
  if (Get(hashed, step) != (1 << counter_size) - 1) {
    auto i = num_counters_ * step + (hashed & mask_);
    data_[GetIndex(i)] += 1L << GetOffset(i);
    return true;
  }
  return false;
}

template <typename T, typename Hash>
void Sketch<T, Hash, Policy::Bloom>::Reset() {
  for (auto& counter : data_) counter = ((counter >> 1) & 0x77L);
}

template <typename T, typename Hash>
void Sketch<T, Hash, Policy::Bloom>::Clear() {
  for (auto& counter : data_) counter = 0;
}

}  // namespace cache::impl::sketch

USERVER_NAMESPACE_END
