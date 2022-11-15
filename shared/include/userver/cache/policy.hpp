#pragma once

USERVER_NAMESPACE_BEGIN

namespace cache {
enum CachePolicy {
  kLRU = 0,
  kLFU,
  kTinyLFU,
  kWTinyLFU
};
enum FrequencySketchPolicy {
  Trivial = 0,
  Bloom,
};
} // namespace cache

USERVER_NAMESPACE_END
