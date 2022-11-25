#pragma once

#include <cstddef>

USERVER_NAMESPACE_BEGIN

namespace cache::impl::sketch {
size_t NextPowerOfTwo(size_t n);
}  // namespace cache::impl::sketch

USERVER_NAMESPACE_END
