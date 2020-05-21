#include <cache/cache_config.hpp>

#include <utils/assert.hpp>
#include <utils/string_to_duration.hpp>
#include <utils/traceful_exception.hpp>

namespace cache {

namespace {

const std::string kUpdateIntervalMs = "update-interval-ms";
const std::string kUpdateJitterMs = "update-jitter-ms";
const std::string kFullUpdateIntervalMs = "full-update-interval-ms";

const std::string kUpdateInterval = "update-interval";
const std::string kUpdateJitter = "update-jitter";
const std::string kFullUpdateInterval = "full-update-interval";
const std::string kFirstUpdateFailOk = "first-update-fail-ok";

const std::string kWays = "ways";
const std::string kSize = "size";
const std::string kLifetime = "lifetime";
const std::string kBackgroundUpdate = "background-update";

const std::string kLifetimeMs = "lifetime-ms";

std::chrono::milliseconds GetDefaultJitterMs(
    const std::chrono::milliseconds& interval) {
  return interval / 10;
}

std::chrono::milliseconds JsonToMs(const formats::json::Value& json) {
  return std::chrono::milliseconds{json.As<uint64_t>()};
}

const std::string kUpdateTypes = "update-types";

AllowedUpdateTypes ParseUpdateMode(const components::ComponentConfig& config) {
  const auto update_types_str = config.ParseOptionalString(kUpdateTypes);
  if (!update_types_str) {
    if (config.Yaml().HasMember(kFullUpdateInterval) &&
        config.Yaml().HasMember(kUpdateInterval)) {
      return AllowedUpdateTypes::kFullAndIncremental;
    } else {
      return AllowedUpdateTypes::kOnlyFull;
    }
  } else if (*update_types_str == "full-and-incremental") {
    return AllowedUpdateTypes::kFullAndIncremental;
  } else if (*update_types_str == "only-full") {
    return AllowedUpdateTypes::kOnlyFull;
  } else if (*update_types_str == "only-incremental") {
    return AllowedUpdateTypes::kOnlyIncremental;
  }

  throw std::logic_error("Invalid update types '" + *update_types_str +
                         "' for cache '" + config.Name() + '\'');
}

}  // namespace

CacheConfig::CacheConfig(const components::ComponentConfig& config)
    : allowed_update_types(ParseUpdateMode(config)),
      update_interval(config.ParseDuration(kUpdateInterval,
                                           std::chrono::milliseconds::zero())),
      update_jitter(config.ParseDuration(kUpdateJitter,
                                         GetDefaultJitterMs(update_interval))),
      full_update_interval(config.ParseDuration(
          kFullUpdateInterval, std::chrono::milliseconds::zero())),
      allow_first_update_failure(config.ParseBool(kFirstUpdateFailOk, false)) {
  switch (allowed_update_types) {
    case AllowedUpdateTypes::kFullAndIncremental:
      if (!update_interval.count() || !full_update_interval.count()) {
        throw std::logic_error("Both " + kUpdateInterval + " and " +
                               kFullUpdateInterval +
                               " must be set for cache "
                               "'" +
                               config.Name() + '\'');
      }
      if (update_interval >= full_update_interval) {
        LOG_WARNING() << "Incremental updates requested for cache '"
                      << config.Name()
                      << "' but have lower frequency than full updates and "
                         "will never happen. Remove "
                      << kFullUpdateInterval
                      << " config field if this is intended.";
      }
      break;
    case AllowedUpdateTypes::kOnlyFull:
    case AllowedUpdateTypes::kOnlyIncremental:
      if (full_update_interval.count()) {
        throw std::logic_error(kFullUpdateInterval +
                               " config field only be used with "
                               "full-and-incremental updated cache '" +
                               config.Name() + "'. Please rename it to " +
                               kUpdateInterval + '.');
      }
      if (!update_interval.count()) {
        throw std::logic_error(kUpdateInterval + " is not set for cache '" +
                               config.Name() + '\'');
      }
      full_update_interval = update_interval;
      break;
  }
}

CacheConfig::CacheConfig(std::chrono::milliseconds update_interval_,
                         std::chrono::milliseconds update_jitter_,
                         std::chrono::milliseconds full_update_interval_)
    : allowed_update_types(AllowedUpdateTypes::kOnlyFull),
      update_interval(update_interval_),
      update_jitter(update_jitter_),
      full_update_interval(full_update_interval_),
      allow_first_update_failure(false) {
  if (!full_update_interval.count()) {
    if (!update_interval.count()) {
      throw utils::impl::AttachTraceToException(
          std::logic_error("Update interval is not set for cache"));
    }
    full_update_interval = update_interval;
  } else {
    allowed_update_types = AllowedUpdateTypes::kFullAndIncremental;
  }
}

LruCacheConfigStatic::LruCacheConfigStatic(
    const components::ComponentConfig& component_config)
    : config(component_config.ParseUint64(kSize),
             component_config.ParseDuration(kLifetime,
                                            std::chrono::milliseconds::zero()),
             component_config.ParseBool(kBackgroundUpdate, false)
                 ? BackgroundUpdateMode::kEnabled
                 : BackgroundUpdateMode::kDisabled),
      ways(component_config.ParseUint64(kWays)) {
  if (ways <= 0) throw std::runtime_error("cache-ways is non-positive");
  if (config.size <= 0) throw std::runtime_error("cache-size is non-positive");
}

LruCacheConfig::LruCacheConfig(size_t size, std::chrono::milliseconds lifetime,
                               BackgroundUpdateMode background_update)
    : size(size), lifetime(lifetime), background_update(background_update) {}

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
  UASSERT(!ConfigName().empty() || !LruConfigName().empty());

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
std::optional<CacheConfig> CacheConfigSet::GetConfig(
    const std::string& cache_name) const {
  auto it = configs_.find(cache_name);
  if (it == configs_.end()) return std::nullopt;
  return it->second;
}

std::optional<LruCacheConfig> CacheConfigSet::GetLruConfig(
    const std::string& cache_name) const {
  auto it = lru_configs_.find(cache_name);
  if (it == lru_configs_.end()) return std::nullopt;
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
  return LruCacheConfig(json[kSize].As<size_t>(), JsonToMs(json[kLifetimeMs]),
                        json[kBackgroundUpdate].As<bool>(false)
                            ? BackgroundUpdateMode::kEnabled
                            : BackgroundUpdateMode::kDisabled);
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
