#pragma once

USERVER_NAMESPACE_BEGIN

namespace cache {
enum CachePolicy {
  kLRU = 0,
  kLFU = 1
};
}

USERVER_NAMESPACE_END
