#include <userver/cache/impl/hash.hpp>

#include <cmath>

USERVER_NAMESPACE_BEGIN

namespace cache::impl::utils {
template <typename T>
uint32_t Jenkins<T>::operator()(const T& item) {
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
}  // namespace cache::impl::utils

USERVER_NAMESPACE_END