#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

#include <userver/cache/impl/doorkeeper.hpp>
#include <userver/cache/impl/hash.hpp>
#include <userver/cache/policy.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache::impl {
template <typename T, typename Hash = std::hash<T>,
          FrequencySketchPolicy policy = FrequencySketchPolicy::Bloom>
class FrequencySketch {};

// TODO: freq_type refactor
template <typename T, typename Hash>
class FrequencySketch<T, Hash, FrequencySketchPolicy::Bloom> {
  using freq_type = int;

 public:
  explicit FrequencySketch(size_t capacity, const Hash& = std::hash<T>{});
  freq_type GetFrequency(const T& item);
  void RecordAccess(const T& item);
  freq_type Size() { return size_; }
  void Clear();
  void Reset();

 private:
  static constexpr size_t counter_size_ = 4;
  std::vector<uint64_t> table_;
  Hash hash_;
  freq_type size_{0};
  // TODO: generated hashes
  int num_hashes_ = 4;

  freq_type GetCount(const T& item, int step);
  uint32_t GetHash(const T& item, int step);
  int GetIndex(uint32_t hash);
  int GetOffset(uint32_t hash, int step);

  bool TryIncrement(const T& item, int step);
};

template <typename T, typename Hash>
class FrequencySketch<T, Hash, FrequencySketchPolicy::DoorkeeperBloom> {
  using freq_type = int;

 public:
  explicit FrequencySketch(size_t capacity, const Hash& = std::hash<T>{});
  freq_type GetFrequency(const T& item);
  void RecordAccess(const T& item);
  freq_type Size();
  void Clear();

 private:
  Doorkeeper<T, Hash> proxy_;
  FrequencySketch<T, Hash, FrequencySketchPolicy::Bloom> main_;
};

// TODO: capacity - ? (space optimize)
template <typename T, typename Hash>
FrequencySketch<T, Hash, FrequencySketchPolicy::Bloom>::FrequencySketch(
    size_t capacity, const Hash& hash)
    : table_(tools::NextPowerOfTwo(capacity) >> 2), hash_(hash) {}

template <typename T, typename Hash>
int FrequencySketch<T, Hash, FrequencySketchPolicy::Bloom>::GetFrequency(
    const T& item) {
  auto freq = std::numeric_limits<freq_type>::max();
  for (int i = 0; i < num_hashes_; i++)
    freq = std::min(freq, GetCount(item, i));
  return freq;
}

template <typename T, typename Hash>
int FrequencySketch<T, Hash, FrequencySketchPolicy::Bloom>::GetCount(
    const T& item, int step) {
  auto hash = GetHash(item, step);
  int index = GetIndex(hash);
  int offset = GetOffset(hash, step);
  return (table_[index] >> offset) & ((1 << counter_size_) - 1);
}

template <typename T, typename Hash>
uint32_t FrequencySketch<T, Hash, FrequencySketchPolicy::Bloom>::GetHash(
    const T& item, int step) {
  static constexpr uint64_t seeds[] = {0xc3a5c85c97cb3127L, 0xb492b66fbe98f273L,
                                       0x9ae16a3b2f90404fL,
                                       0xcbf29ce484222325L};
  uint64_t hash = seeds[step] * hash_(item);
  hash += hash >> 32;
  return hash;
}

template <typename T, typename Hash>
int FrequencySketch<T, Hash, FrequencySketchPolicy::Bloom>::GetIndex(
    uint32_t hash) {
  return hash & (table_.size() - 1);
}

template <typename T, typename Hash>
int FrequencySketch<T, Hash, FrequencySketchPolicy::Bloom>::GetOffset(
    uint32_t hash, int step) {
  return (((hash & 3) << 2) + step) << 2;
}

template <typename T, typename Hash>
void FrequencySketch<T, Hash, FrequencySketchPolicy::Bloom>::RecordAccess(
    const T& item) {
  auto was_added = false;
  for (int i = 0; i < num_hashes_; i++) was_added |= TryIncrement(item, i);
  if (was_added) size_++;
}

template <typename T, typename Hash>
bool FrequencySketch<T, Hash, FrequencySketchPolicy::Bloom>::TryIncrement(
    const T& item, int step) {
  auto hash = GetHash(item, step);
  int index = GetIndex(hash);
  int offset = GetOffset(hash, step);
  if (GetCount(item, step) != ((1 << counter_size_) - 1)) {
    table_[index] += 1L << offset;
    return true;
  }
  return false;
}

template <typename T, typename Hash>
void FrequencySketch<T, Hash, FrequencySketchPolicy::Bloom>::Reset() {
  for (auto& counters : table_)
    counters = (counters >> 1) & 0x7777777777777777L;
  size_ = (size_ >> 1);
}

template <typename T, typename Hash>
void FrequencySketch<T, Hash, FrequencySketchPolicy::Bloom>::Clear() {
  for (auto& counter : table_) {
    counter = 0;
  }
  size_ = 0;
}

template <typename T, typename Hash>
FrequencySketch<T, Hash, FrequencySketchPolicy::DoorkeeperBloom>::
    FrequencySketch(size_t capacity, const Hash& hash)
    : proxy_(capacity, hash), main_(capacity, hash) {}

template <typename T, typename Hash>
int FrequencySketch<T, Hash, FrequencySketchPolicy::DoorkeeperBloom>::
    GetFrequency(const T& item) {
  if (proxy_.Contains(item)) return main_.GetFrequency(item) + 1;
  return main_.GetFrequency(item);
}

template <typename T, typename Hash>
void FrequencySketch<T, Hash, FrequencySketchPolicy::DoorkeeperBloom>::
    RecordAccess(const T& item) {
  if (!proxy_.Contains(item)) {
    proxy_.Put(item);
    return;
  }
  return main_.RecordAccess(item);
}

template <typename T, typename Hash>
void FrequencySketch<T, Hash, FrequencySketchPolicy::DoorkeeperBloom>::Clear() {
  proxy_.Clear();
  main_.Clear();
}

template <typename T, typename Hash>
int FrequencySketch<T, Hash, FrequencySketchPolicy::DoorkeeperBloom>::Size() {
  return main_.Size();
}

}  // namespace cache::impl

USERVER_NAMESPACE_END
