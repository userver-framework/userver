#pragma once

#include <userver/cache/policy.hpp>
#include <userver/cache/impl/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache::impl {

template <typename T, typename U, typename Hash, typename Equal>
class LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU> {
 public:
  explicit LruBase(size_t max_size, const Hash hash, const Equal& equal);
 private:
  LruBase<T, U, Hash, Equal, CachePolicy::kLRU> main_;
};

}

USERVER_NAMESPACE_END
