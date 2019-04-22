#include <cache/cache_config.hpp>

#include <utils/assert.hpp>
#include <utils/string_to_duration.hpp>

namespace cache {

namespace {

const std::string kUpdateIntervalMs = "update-interval-ms";
const std::string kUpdateJitterMs = "update-jitter-ms";
const std::string kFullUpdateIntervalMs = "full-update-interval-ms";

const std::string kUpdateInterval = "update-interval";
const std::string kUpdateJitter = "update-jitter";
const std::string kFullUpdateInterval = "full-update-interval";

const std::string kWays = "ways";
const std::string kSize = "size";
const std::string kLifetime = "lifetime";

const std::string kLifetimeMs = "lifetime-ms";

std::chrono::milliseconds GetDefaultJitterMs(
    const std::chrono::milliseconds& interval) {
  return interval / 10;
}

std::chrono::milliseconds JsonToMs(const formats::json::Value& json) {
  return std::chrono::milliseconds{json.As<uint64_t>()};
}

}  // namespace

CacheConfig::CacheConfig(const components::ComponentConfig& config)
    : update_interval_(config.ParseDuration(kUpdateInterval)),
      update_jitter_(config.ParseDuration(
          kUpdateJitter, GetDefaultJitterMs(update_interval_))),
      full_update_interval_(config.ParseDuration(
          kFullUpdateInterval, std::chrono::milliseconds::zero())) {}

CacheConfig::CacheConfig(std::chrono::milliseconds update_interval,
                         std::chrono::milliseconds update_jitter,
                         std::chrono::milliseconds full_update_interval)
    : update_interval_(update_interval),
      update_jitter_(update_jitter),
      full_update_interval_(full_update_interval) {}

LruCacheConfigStatic::LruCacheConfigStatic(
    const components::ComponentConfig& component_config)
    : config(component_config.ParseUint64(kSize),
             component_config.ParseDuration(kLifetime,
                                            std::chrono::milliseconds::zero())),
      ways(component_config.ParseUint64(kWays)) {
  if (ways <= 0) throw std::runtime_error("cache-ways is non-positive");
  if (config.size <= 0) throw std::runtime_error("cache-size is non-positive");
}

LruCacheConfig::LruCacheConfig(size_t size, std::chrono::milliseconds lifetime)
    : size(size), lifetime(lifetime) {}

size_t LruCacheConfigStatic::GetWaySize() const {
  auto way_size = config.size / ways;
  return way_size == 0 ? 1 : way_size;
}

LruCacheConfigStatic LruCacheConfigStatic::MergeWith(
    const LruCacheConfig& other) const {
  LruCacheConfigStatic config(*this);
  config.config = other;
  return config;
}

CacheConfigSet::CacheConfigSet(const taxi_config::DocsMap& docs_map) {
  UASSERT(!ConfigName().empty());

  const auto& config_name = ConfigName();
  if (!config_name.empty()) {
    auto caches_json = docs_map.Get(config_name);
    for (auto it = caches_json.begin(); it != caches_json.end(); ++it) {
      configs_.emplace(it.GetName(), ParseConfig(*it));
    }
  }

  const auto& lru_config_name = LruConfigName();
  if (!lru_config_name.empty()) {
    auto lru_caches_json = docs_map.Get(lru_config_name);
    for (auto it = lru_caches_json.begin(); it != lru_caches_json.end(); ++it) {
      lru_configs_.emplace(it.GetName(), ParseLruConfig(*it));
    }
  }
}

/// Get config for cache
boost::optional<CacheConfig> CacheConfigSet::GetConfig(
    const std::string& cache_name) const {
  auto it = configs_.find(cache_name);
  if (it == configs_.end()) return boost::none;
  return it->second;
}

boost::optional<LruCacheConfig> CacheConfigSet::GetLruConfig(
    const std::string& cache_name) const {
  auto it = lru_configs_.find(cache_name);
  if (it == lru_configs_.end()) return boost::none;
  return it->second;
}

CacheConfig CacheConfigSet::ParseConfig(const formats::json::Value& json) {
  CacheConfig config(JsonToMs(json[kUpdateIntervalMs]),
                     JsonToMs(json[kUpdateJitterMs]),
                     JsonToMs(json[kFullUpdateIntervalMs]));

  return config;
}

LruCacheConfig CacheConfigSet::ParseLruConfig(
    const formats::json::Value& json) {
  return LruCacheConfig(json[kSize].As<size_t>(), JsonToMs(json[kLifetimeMs]));
}

bool CacheConfigSet::IsConfigEnabled() { return !ConfigName().empty(); }

bool CacheConfigSet::IsLruConfigEnabled() { return !LruConfigName().empty(); }

void CacheConfigSet::SetConfigName(const std::string& name) {
  ConfigName() = name;
}

std::string& CacheConfigSet::ConfigName() {
  static std::string name;
  return name;
}

void CacheConfigSet::SetLruConfigName(const std::string& name) {
  LruConfigName() = name;
}

std::string& CacheConfigSet::LruConfigName() {
  static std::string name;
  return name;
}

}  // namespace cache
