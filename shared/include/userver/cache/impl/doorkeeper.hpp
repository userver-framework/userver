#pragma once

#include <cmath>
#include <cstdint>
#include <vector>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache::impl {
// hash_i = hash1 + i * hash2 the distribution grows uneven for some inputs!!!
// (but simple)
template <typename T, typename Hash = std::hash<T>>
class Doorkeeper {
 public:
  Doorkeeper(size_t capacity, const Hash& = std::hash<T>{});
  void Put(const T&);
  bool Contains(const T&);
  void Reset();
  void Clear();

 private:
  std::vector<bool> table_;
  size_t num_hashes_ = 4;
  Hash hash_;

  std::pair<uint32_t, uint32_t> GetHash(const T&);
};

template <typename T, typename Hash>
Doorkeeper<T, Hash>::Doorkeeper(size_t capacity, const Hash& hash)
    : table_(capacity), hash_(hash) {
  UINVARIANT(false, "not implemented yet");
    }

template <typename T, typename Hash>
std::pair<uint32_t, uint32_t> Doorkeeper<T, Hash>::GetHash(const T& item) {
  uint64_t hash = hash_(item) * 0xc3a5c85c97cb3127L;
  hash += hash >> 32;
  uint32_t h1 = hash;
  uint32_t h2 = hash >> 32;
  return {h1, h2};
}

template <typename T, typename Hash>
void Doorkeeper<T, Hash>::Put(const T& item) {
  auto [h1, h2] = GetHash(item);
  for (size_t i = 0; i < num_hashes_; i++)
    table_[(h1 + i * h2) % table_.size()] = true;
}

template <typename T, typename Hash>
bool Doorkeeper<T, Hash>::Contains(const T& item) {
  auto [h1, h2] = GetHash(item);
  bool res = true;
  for (size_t i = 0; i < num_hashes_; i++)
    res &= table_[(h1 + i * h2) % table_.size()];
  return res;
}

template <typename T, typename Hash>
void Doorkeeper<T, Hash>::Reset() {
  table_.assign(table_.size(), false);
}

template <typename T, typename Hash>
void Doorkeeper<T, Hash>::Clear() {
  table_.assign(table_.size(), false);
}
}  // namespace cache::impl

USERVER_NAMESPACE_END
