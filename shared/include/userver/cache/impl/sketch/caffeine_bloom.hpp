#pragma once

#include <userver/cache/impl/sketch/policy.hpp>
#include <userver/cache/impl/sketch/tools.hpp>
#include "userver/utils/assert.hpp"

USERVER_NAMESPACE_BEGIN

namespace cache::impl::sketch {

template <typename T, typename Hash>
class Sketch<T, Hash, Policy::CaffeineBloom> {
  using freq_type = int;

 public:
  explicit Sketch(size_t capacity, const Hash& = std::hash<T>{},
                  int access_count_limit_rate = 10);
  freq_type Estimate(const T& item);
  void Increment(const T& item);
  freq_type Size() { return size_; }
  void Clear();

 private:
  std::vector<uint64_t> table_;
  Hash hash_;
  size_t size_{0};
  static constexpr size_t counter_size_ = 4;
  int32_t access_count_limit_rate_ = 10;
  static constexpr int num_hashes_ = 4;
  uint64_t seed_[num_hashes_] = {0xc3a5c85c97cb3127L, 0xb492b66fbe98f273L,
                                 0x9ae16a3b2f90404fL, 0xcbf29ce484222325L};
  uint64_t reset_mask_ = 0x7777777777777777L;
  uint64_t one_mask_ = 0x1111111111111111L;
  int32_t sample_size_;
  int32_t table_mask_;

  int32_t Spread(u_int32_t x);
  int32_t IndexOf(int32_t item, int32_t i);
  bool IncrementAt(int32_t i, int32_t j);
  void Reset();
  uint32_t SetBitCount(int64_t x);
};

template <typename T, typename Hash>
Sketch<T, Hash, Policy::CaffeineBloom>::Sketch(size_t capacity,
                                               const Hash& hash,
                                               int access_count_limit_rate)
    : hash_(hash), access_count_limit_rate_(access_count_limit_rate) {
  UINVARIANT(false, "not implemented yet");
  uint64_t ceiling_two_power = 1;
  while (ceiling_two_power < capacity) {
    ceiling_two_power <<= 1;
  }
  table_.resize(ceiling_two_power);
  table_mask_ = std::max(1, static_cast<int32_t>(table_.size() - 1));
  int32_t maximum = std::min(static_cast<int32_t>(capacity),
                             std::numeric_limits<int32_t>::max() >> 1);
  sample_size_ = maximum == 0 ? 10 : 10 * maximum;
}

template <typename T, typename Hash>
int Sketch<T, Hash, Policy::CaffeineBloom>::Estimate(const T& item) {
  int32_t hash = Spread(hash_(item));
  int32_t start = (hash & 3) << 2;
  int32_t frequency = std::numeric_limits<int32_t>::max();
  for (size_t i = 0; i < num_hashes_; ++i) {
    int32_t index = IndexOf(hash, i);
    auto count =
        static_cast<int32_t>((table_[index] >> ((start + i) << 2)) & 0xfL);
    frequency = std::min(frequency, count);
  }
  return frequency;
}

template <typename T, typename Hash>
void Sketch<T, Hash, Policy::CaffeineBloom>::Increment(const T& item) {
  int32_t hash = Spread(hash_(item));
  int32_t start = (hash & 3) << 2;
  std::vector<int32_t> indexes(num_hashes_);
  for (size_t i = 0; i < num_hashes_; ++i) {
    indexes[i] = IndexOf(hash, i);
  }
  bool added = IncrementAt(indexes[0], start);
  for (size_t i = 1; i < num_hashes_; ++i) {
    added |= IncrementAt(indexes[i], start + static_cast<int32_t>(i));
  }

  if (added && ++size_ == sample_size_) {
    Reset();
  }
}

template <typename T, typename Hash>
void Sketch<T, Hash, Policy::CaffeineBloom>::Reset() {
  u_int32_t count = 0;
  for (auto& value : table_) {
    count += SetBitCount(value & one_mask_);
    value = (value >> 1) & reset_mask_;
  }
  size_ = (size_ - (count >> 2)) >> 1;
}

template <typename T, typename Hash>
bool Sketch<T, Hash, Policy::CaffeineBloom>::IncrementAt(int32_t i, int32_t j) {
  int32_t offset = j << 2;
  int64_t mask = (0xfL << offset);
  if ((table_[i] & mask) != mask) {
    table_[i] += (1L << offset);
    return true;
  }
  return false;
}

template <typename T, typename Hash>
uint32_t Sketch<T, Hash, Policy::CaffeineBloom>::SetBitCount(int64_t x) {
  uint32_t ret = 0;
  while (x != 0) {
    x &= (x - 1);
    ++ret;
  }
  return ret;
}

template <typename T, typename Hash>
int32_t Sketch<T, Hash, Policy::CaffeineBloom>::IndexOf(int32_t item,
                                                        int32_t i) {
  int64_t hash = (item + seed_[i]) * seed_[i];
  hash += hash >> 32;
  return static_cast<int32_t>(hash) & table_mask_;
}

template <typename T, typename Hash>
int32_t Sketch<T, Hash, Policy::CaffeineBloom>::Spread(u_int32_t x) {
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  return (x >> 16) ^ x;
}

}  // namespace cache::impl::sketch

USERVER_NAMESPACE_END