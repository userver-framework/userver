#include <userver/cache/impl/sketch/tools.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache::impl::sketch {
size_t NextPowerOfTwo(size_t n) {
  UASSERT(n > 0);
  n--;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  n++;
  return n;
}
}  // namespace cache::impl::sketch

USERVER_NAMESPACE_END
