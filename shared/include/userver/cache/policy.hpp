#pragma once

USERVER_NAMESPACE_BEGIN

namespace cache {
enum CachePolicy { kLRU = 0, kSLRU, kLFU, kTinyLFU, kWTinyLFU };
}  // namespace cache

USERVER_NAMESPACE_END
