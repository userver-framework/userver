#pragma once

#include <boost/dynamic_bitset.hpp>

#include <userver/cache/impl/sketch/bloom.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache::impl::sketch {
// TODO: pass hashed value
template <typename T, typename Hash>
class Sketch<T, Hash, Policy::Doorkeeper> {
 public:
  explicit Sketch(size_t num_counters, const Hash& hash = Hash{});
  int Estimate(const T& item);
  bool Increment(const T& item);
  void Clear();
  void Reset();

 private:
  boost::dynamic_bitset<> data_;
  Hash hash_;
  size_t mask_;
  Sketch<T, Hash, Policy::Bloom> main_;

  static constexpr size_t num_hashes = 4;
  // TODO: random seeds
  static constexpr uint64_t seeds[] = {0xc3a5c85c97cb3127L, 0xb492b66fbe98f273L,
                                       0x9ae16a3b2f90404fL,
                                       0xcbf29ce484222325L};

  void Put(const T&);
  bool Contains(const T&);
};

template <typename T, typename Hash>
Sketch<T, Hash, Policy::Doorkeeper>::Sketch(size_t num_counters,
                                            const Hash& hash)
    : data_(NextPowerOfTwo(num_counters) << 2),
      hash_(hash),
      mask_((NextPowerOfTwo(num_counters) << 2) - 1),
      main_(num_counters, hash) {
  UASSERT(num_counters > 0);
}

template <typename T, typename Hash>
int Sketch<T, Hash, Policy::Doorkeeper>::Estimate(const T& item) {
  if (Contains(item)) return main_.Estimate(item) + 1;
  return main_.Estimate(item);
}

template <typename T, typename Hash>
void Sketch<T, Hash, Policy::Doorkeeper>::Clear() {
  data_.reset();
  main_.Clear();
}

template <typename T, typename Hash>
void Sketch<T, Hash, Policy::Doorkeeper>::Reset() {
  data_.reset();
  main_.Reset();
}

template <typename T, typename Hash>
bool Sketch<T, Hash, Policy::Doorkeeper>::Increment(const T& item) {
  if (!Contains(item)) {
    Put(item);
    return true;
  }
  return main_.Increment(item);
}

template <typename T, typename Hash>
bool Sketch<T, Hash, Policy::Doorkeeper>::Contains(const T& item) {
  bool result = true;
  auto hashed = hash_(item);
  for (unsigned long seed : seeds) result &= data_[(hashed ^ seed) & mask_];
  return result;
}

template <typename T, typename Hash>
void Sketch<T, Hash, Policy::Doorkeeper>::Put(const T& item) {
  auto hashed = hash_(item);
  for (unsigned long seed : seeds) data_[(hashed ^ seed) & mask_] = true;
}

}  // namespace cache::impl::sketch

USERVER_NAMESPACE_END
