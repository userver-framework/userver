#include <cache/cache_config.hpp>
#include <cassert>

namespace components {

namespace {

const std::string kUpdateIntervalMs = "update-interval-ms";
const std::string kUpdateJitterMs = "update-jitter-ms";
const std::string kFullUpdateInterval = "full-update-interval-ms";

uint64_t GetDefaultJitterMs(const std::chrono::milliseconds& interval) {
  return (interval / 10).count();
}

}  // namespace

CacheConfig::CacheConfig(const ComponentConfig& config)
    : update_interval_(config.ParseUint64(kUpdateIntervalMs)),
      update_jitter_(config.ParseUint64(kUpdateJitterMs,
                                        GetDefaultJitterMs(update_interval_))),
      full_update_interval_(config.ParseUint64(kFullUpdateInterval, 0)) {}

CacheConfig::CacheConfig(std::chrono::milliseconds update_interval,
                         std::chrono::milliseconds update_jitter,
                         std::chrono::milliseconds full_update_interval)
    : update_interval_(update_interval),
      update_jitter_(update_jitter),
      full_update_interval_(full_update_interval) {}

CacheConfigSet::CacheConfigSet(const taxi_config::DocsMap& docs_map) {
  assert(!ConfigName().empty());

  auto caches_json = docs_map.Get(ConfigName());
  for (auto it = caches_json.begin(); it != caches_json.end(); ++it) {
    configs_.emplace(it.GetName(), ParseConfig(*it));
  }
}

/// Get config for cache
boost::optional<CacheConfig> CacheConfigSet::GetConfig(
    const std::string& cache_name) const {
  auto it = configs_.find(cache_name);
  if (it == configs_.end()) return boost::none;
  return it->second;
}

CacheConfig CacheConfigSet::ParseConfig(formats::json::Value json) {
  CacheConfig config(
      std::chrono::seconds(json[kUpdateIntervalMs].As<uint64_t>()),
      std::chrono::seconds(json[kUpdateJitterMs].As<uint64_t>()),
      std::chrono::seconds(json[kFullUpdateInterval].As<uint64_t>()));

  return config;
}

bool CacheConfigSet::IsConfigEnabled() { return !ConfigName().empty(); }

void CacheConfigSet::SetConfigName(const std::string& name) {
  ConfigName() = name;
}

std::string& CacheConfigSet::ConfigName() {
  static std::string name;
  return name;
}

}  // namespace components
