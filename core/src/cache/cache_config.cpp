#include <cache/cache_config.hpp>

#include <logging/log.hpp>
#include <utils/assert.hpp>
#include <utils/string_to_duration.hpp>
#include <utils/traceful_exception.hpp>

namespace cache {

namespace {

constexpr std::string_view kUpdateInterval = "update-interval";
constexpr std::string_view kUpdateJitter = "update-jitter";
constexpr std::string_view kFullUpdateInterval = "full-update-interval";
constexpr std::string_view kFirstUpdateFailOk = "first-update-fail-ok";
constexpr std::string_view kCleanupInterval = "additional-cleanup-interval";

constexpr std::string_view kWays = "ways";
constexpr std::string_view kSize = "size";
constexpr std::string_view kLifetime = "lifetime";
constexpr std::string_view kBackgroundUpdate = "background-update";

std::chrono::milliseconds GetDefaultJitterMs(
    const std::chrono::milliseconds& interval) {
  return interval / 10;
}

std::chrono::milliseconds JsonToMs(const formats::json::Value& json) {
  return std::chrono::milliseconds{json.As<uint64_t>()};
}

AllowedUpdateTypes ParseUpdateMode(const components::ComponentConfig& config) {
  const auto update_types_str =
      config["update-types"].As<std::optional<std::string>>();
  if (!update_types_str) {
    if (config.HasMember(kFullUpdateInterval) &&
        config.HasMember(kUpdateInterval)) {
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
      update_interval(config[kUpdateInterval].As<std::chrono::milliseconds>(0)),
      update_jitter(config[kUpdateJitter].As<std::chrono::milliseconds>(
          GetDefaultJitterMs(update_interval))),
      full_update_interval(
          config[kFullUpdateInterval].As<std::chrono::milliseconds>(0)),
      cleanup_interval(config[kCleanupInterval].As<std::chrono::milliseconds>(
          std::chrono::seconds(10))),
      allow_first_update_failure(config[kFirstUpdateFailOk].As<bool>(false)) {
  switch (allowed_update_types) {
    case AllowedUpdateTypes::kFullAndIncremental:
      if (!update_interval.count() || !full_update_interval.count()) {
        throw std::logic_error(
            fmt::format(FMT_STRING("Both {} and {} must be set for cache '{}'"),
                        kUpdateInterval, kFullUpdateInterval, config.Name()));
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
        throw std::logic_error(fmt::format(
            FMT_STRING(
                "{} config field must only be used with full-and-incremental "
                "updated cache '{}'. Please rename it to {}."),
            kFullUpdateInterval, config.Name(), kUpdateInterval));
      }
      if (!update_interval.count()) {
        throw std::logic_error(
            fmt::format(FMT_STRING("{} is not set for cache '{}'"),
                        kUpdateInterval, config.Name()));
      }
      full_update_interval = update_interval;
      break;
  }
}

CacheConfig::CacheConfig(std::chrono::milliseconds update_interval,
                         std::chrono::milliseconds update_jitter,
                         std::chrono::milliseconds full_update_interval,
                         std::chrono::milliseconds cleanup_interval)
    : allowed_update_types(AllowedUpdateTypes::kOnlyFull),
      update_interval(update_interval),
      update_jitter(update_jitter),
      full_update_interval(full_update_interval),
      cleanup_interval(cleanup_interval),
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
    : config(component_config[kSize].As<size_t>(),
             component_config[kLifetime].As<std::chrono::milliseconds>(0),
             component_config[kBackgroundUpdate].As<bool>(false)
                 ? BackgroundUpdateMode::kEnabled
                 : BackgroundUpdateMode::kDisabled),
      ways(component_config[kWays].As<size_t>()) {
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
    for (const auto& [name, value] : Items(caches_json)) {
      configs_.emplace(name, ParseConfig(value));
    }
  }

  const auto& lru_config_name = LruConfigName();
  if (!lru_config_name.empty()) {
    auto lru_caches_json = docs_map.Get(lru_config_name);
    for (const auto& [name, value] : Items(lru_caches_json)) {
      lru_configs_.emplace(name, ParseLruConfig(value));
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
  CacheConfig config(
      JsonToMs(json["update-interval-ms"]), JsonToMs(json["update-jitter-ms"]),
      JsonToMs(json["full-update-interval-ms"]),
      std::chrono::milliseconds{
          json["additional-cleanup-interval-ms"].As<uint64_t>(10000)});

  return config;
}

LruCacheConfig CacheConfigSet::ParseLruConfig(
    const formats::json::Value& json) {
  return LruCacheConfig(json[kSize].As<size_t>(), JsonToMs(json["lifetime-ms"]),
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
