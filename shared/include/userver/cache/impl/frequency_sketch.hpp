#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

#include <userver/cache/policy.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache::impl {
template <typename T,
          FrequencySketchPolicy policy = FrequencySketchPolicy::Bloom>
class FrequencySketch {};

// TODO: think about hash
template <typename T>
class FrequencySketch<T, FrequencySketchPolicy::Bloom> {
  using freq_type = int;

 public:
  explicit FrequencySketch(std::size_t capacity,
                           int access_count_limit_rate = 10);
  freq_type GetFrequency(const T& item);
  void RecordAccess(const T& item);
  freq_type Size() { return size_; }
  void Clear();
 private:
  std::vector<uint64_t> table_;
  freq_type size_{0};
  static constexpr std::size_t counter_size_ = 4;
  int access_count_limit_rate_ = 10;
  // TODO: generated hashes
  int num_hashes_ = 4;

  freq_type GetCount(const T& item, int step);
  uint32_t GetHash(const T& item);
  uint32_t GetHash(const T& item, int step);
  int GetIndex(uint32_t hash);
  int GetOffset(uint32_t hash, int step);
  freq_type GetSamplingSize();

  bool TryIncrement(const T& item, int step);
  void Reset();
};

// TODO: capacity - ? (space optimize)
template <typename T>
FrequencySketch<T, FrequencySketchPolicy::Bloom>::FrequencySketch(
    std::size_t capacity, int access_count_limit_rate)
    : table_(capacity),
      access_count_limit_rate_(access_count_limit_rate) {}

template <typename T>
int FrequencySketch<T, FrequencySketchPolicy::Bloom>::GetFrequency(
    const T& item) {
  auto freq = std::numeric_limits<freq_type>::max();
  for (int i = 0; i < num_hashes_; i++)
    freq = std::min(freq, GetCount(item, i));
  return freq;
}

template <typename T>
int FrequencySketch<T, FrequencySketchPolicy::Bloom>::GetCount(const T& item,
                                                               int step) {
  auto hash = GetHash(item, step);
  int index = GetIndex(hash);
  int offset = GetOffset(hash, step);
  return (table_[index] >> offset) & ((1 << counter_size_) - 1);
}

template <typename T>
uint32_t FrequencySketch<T, FrequencySketchPolicy::Bloom>::GetHash(
    const T& item) {
  const char* data = reinterpret_cast<const char*>(&item);
  uint32_t hash = 0;

  for (size_t i = 0; i < static_cast<size_t>(sizeof item); ++i) {
    hash += data[i];
    hash += hash << 10;
    hash ^= hash >> 6;
  }

  hash += hash << 3;
  hash ^= hash >> 11;
  hash += hash << 15;

  return hash;
}

template <typename T>
uint32_t FrequencySketch<T, FrequencySketchPolicy::Bloom>::GetHash(
    const T& item, int step) {
  static constexpr uint64_t seeds[] = {0xc3a5c85c97cb3127L, 0xb492b66fbe98f273L,
                                       0x9ae16a3b2f90404fL,
                                       0xcbf29ce484222325L};
  uint64_t hash = seeds[step] * GetHash(item);
  hash += hash >> 32;
  return hash;
}

template <typename T>
int FrequencySketch<T, FrequencySketchPolicy::Bloom>::GetIndex(uint32_t hash) {
  return hash & (table_.size() - 1);
}

template <typename T>
int FrequencySketch<T, FrequencySketchPolicy::Bloom>::GetOffset(uint32_t hash,
                                                                int step) {
  return (((hash & 3) << 2) + step) << 2;
}

template <typename T>
void FrequencySketch<T, FrequencySketchPolicy::Bloom>::RecordAccess(
    const T& item) {
  auto was_added = false;
  for (int i = 0; i < num_hashes_; i++) was_added |= TryIncrement(item, i);
  if (was_added && (++size_ == GetSamplingSize())) Reset();
}

template <typename T>
bool FrequencySketch<T, FrequencySketchPolicy::Bloom>::TryIncrement(
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

template <typename T>
void FrequencySketch<T, FrequencySketchPolicy::Bloom>::Reset() {
  for (auto& counters : table_)
    counters = (counters >> 1) & 0x7777777777777777L;
  size_ = (size_ >> 1);
}

// TODO: think about it
template <typename T>
int FrequencySketch<T, FrequencySketchPolicy::Bloom>::GetSamplingSize() {
  return static_cast<freq_type>(table_.size() * access_count_limit_rate_);
}
template <typename T>
void FrequencySketch<T, FrequencySketchPolicy::Bloom>::Clear() {
  for (auto& counter : table_) {
    counter = 0;
  }
  size_ = 0;
}

}  // namespace cache::impl

USERVER_NAMESPACE_END
