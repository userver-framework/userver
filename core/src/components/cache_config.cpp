#include <components/cache_config.hpp>

namespace components {

namespace {

const uint64_t kDefaultUpdateIntervalMs = 5 * 60 * 1000;

uint64_t GetDefaultJitterMs(const std::chrono::milliseconds& interval) {
  return (interval / 10).count();
}

}  // namespace

CacheConfig::CacheConfig(const ComponentConfig& config)
    : update_interval_(
          config.ParseUint64("update_interval_ms", kDefaultUpdateIntervalMs)),
      update_jitter_(config.ParseUint64("update_jitter_ms",
                                        GetDefaultJitterMs(update_interval_))),
      full_update_interval_(config.ParseUint64("full_update_interval_ms", 0)) {}

}  // namespace components
