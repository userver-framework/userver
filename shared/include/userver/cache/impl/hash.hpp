#pragma once

#include <cstdint>

USERVER_NAMESPACE_BEGIN

namespace cache::impl::internal {
template <typename T>
struct Jenkins {
  uint32_t operator()(const T& item) {
    const char* data = reinterpret_cast<const char*>(&item);
    uint32_t hash = 0;

    for (std::size_t i = 0; i < static_cast<size_t>(sizeof item); ++i) {
      hash += data[i];
      hash += hash << 10;
      hash ^= hash >> 6;
    }

    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;

    return hash;
  }
};
}  // namespace cache::impl::internal

USERVER_NAMESPACE_END