#include <cache/cache_config.hpp>

#include <logging/log.hpp>
#include <utils/assert.hpp>
#include <utils/string_to_duration.hpp>
#include <utils/traceful_exception.hpp>

namespace cache {

namespace {

constexpr std::string_view kUpdateIntervalMs = "update-interval-ms";
constexpr std::string_view kUpdateJitterMs = "update-jitter-ms";
constexpr std::string_view kFullUpdateIntervalMs = "full-update-interval-ms";
constexpr std::string_view kCleanupIntervalMs =
    "additional-cleanup-interval-ms";

constexpr std::string_view kUpdateInterval = "update-interval";
constexpr std::string_view kUpdateJitter = "update-jitter";
constexpr std::string_view kFullUpdateInterval = "full-update-interval";
constexpr std::string_view kCleanupInterval = "additional-cleanup-interval";

constexpr std::string_view kFirstUpdateFailOk = "first-update-fail-ok";
constexpr std::string_view kUpdateTypes = "update-types";
constexpr std::string_view kForcePeriodicUpdates =
    "testsuite-force-periodic-update";
constexpr std::string_view kTestsuiteDisableUpdates =
    "testsuite-disable-updates";
constexpr std::string_view kConfigSettings = "config-settings";

constexpr std::string_view kWays = "ways";
constexpr std::string_view kSize = "size";
constexpr std::string_view kLifetime = "lifetime";
constexpr std::string_view kBackgroundUpdate = "background-update";
constexpr std::string_view kLifetimeMs = "lifetime-ms";

constexpr std::string_view kDump = "dump";
constexpr std::string_view kDumpsEnabled = "enable";
constexpr std::string_view kDumpDirectory = "userver-cache-dump-path";
constexpr std::string_view kMinDumpInterval = "min-interval";
constexpr std::string_view kFsTaskProcessor = "fs-task-processor";
constexpr std::string_view kDumpFormatVersion = "format-version";
constexpr std::string_view kMaxDumpCount = "max-count";
constexpr std::string_view kMaxDumpAge = "max-age";
constexpr std::string_view kWorldReadable = "world-readable";

constexpr auto kDefaultCleanupInterval = std::chrono::seconds{10};
constexpr auto kDefaultFsTaskProcessor = std::string_view{"fs-task-processor"};
constexpr auto kDefaultDumpFormatVersion = uint64_t{0};
constexpr auto kDefaultMaxDumpCount = uint64_t{2};

std::chrono::milliseconds GetDefaultJitter(std::chrono::milliseconds interval) {
  return interval / 10;
}

std::chrono::milliseconds ParseMs(
    const formats::json::Value& value,
    std::optional<std::chrono::milliseconds> default_value = {}) {
  const auto result = std::chrono::milliseconds{
      default_value ? value.As<int64_t>(default_value->count())
                    : value.As<int64_t>()};
  if (result < std::chrono::milliseconds::zero()) {
    throw formats::json::ParseException("Negative duration");
  }
  return result;
}

AllowedUpdateTypes ParseUpdateMode(const yaml_config::YamlConfig& config) {
  const auto update_types_str =
      config[kUpdateTypes].As<std::optional<std::string>>();
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

  throw std::logic_error(fmt::format("Invalid update types '{}' at '{}'",
                                     *update_types_str, config.GetPath()));
}

std::string ParseDumpDirectory(const components::ComponentConfig& config) {
  const auto dump_root = config.ConfigVars()[kDumpDirectory].As<std::string>();
  return fmt::format("{}/{}", dump_root, config.Name());
}

}  // namespace

CacheConfig::CacheConfig(const components::ComponentConfig& config)
    : update_interval(config[kUpdateInterval].As<std::chrono::milliseconds>(0)),
      update_jitter(config[kUpdateJitter].As<std::chrono::milliseconds>(
          GetDefaultJitter(update_interval))),
      full_update_interval(
          config[kFullUpdateInterval].As<std::chrono::milliseconds>(0)),
      cleanup_interval(config[kCleanupInterval].As<std::chrono::milliseconds>(
          kDefaultCleanupInterval)) {}

CacheConfig::CacheConfig(const formats::json::Value& value)
    : update_interval(ParseMs(value[kUpdateIntervalMs])),
      update_jitter(ParseMs(value[kUpdateJitterMs])),
      full_update_interval(ParseMs(value[kFullUpdateIntervalMs])),
      cleanup_interval(
          ParseMs(value[kCleanupIntervalMs], kDefaultCleanupInterval)) {
  if (!update_interval.count() && !full_update_interval.count()) {
    throw utils::impl::AttachTraceToException(
        std::logic_error("Update interval is not set for cache"));
  } else if (!full_update_interval.count()) {
    full_update_interval = update_interval;
  } else if (!update_interval.count()) {
    update_interval = full_update_interval;
  }

  if (update_jitter > update_interval) {
    update_jitter = GetDefaultJitter(update_interval);
  }
}

CacheConfigStatic::CacheConfigStatic(const components::ComponentConfig& config)
    : CacheConfig(config),
      allowed_update_types(ParseUpdateMode(config)),
      allow_first_update_failure(config[kFirstUpdateFailOk].As<bool>(false)),
      force_periodic_update(
          config[kForcePeriodicUpdates].As<std::optional<bool>>()),
      testsuite_disable_updates(
          config[kTestsuiteDisableUpdates].As<bool>(false)),
      config_updates_enabled(config[kConfigSettings].As<bool>(true)),
      dumps_enabled(config[kDump][kDumpsEnabled].As<bool>(false)),
      dump_directory(ParseDumpDirectory(config)),
      min_dump_interval(
          config[kDump][kMinDumpInterval].As<std::chrono::milliseconds>(0)),
      fs_task_processor(config[kDump][kFsTaskProcessor].As<std::string>(
          kDefaultFsTaskProcessor)),
      dump_format_version(config[kDump][kDumpFormatVersion].As<uint64_t>(
          kDefaultDumpFormatVersion)),
      max_dump_count(
          config[kDump][kMaxDumpCount].As<uint64_t>(kDefaultMaxDumpCount)),
      max_dump_age(config[kDump][kMaxDumpAge]
                       .As<std::optional<std::chrono::milliseconds>>()),
      world_readable(config[kDump][kWorldReadable].As<bool>(false)) {
  switch (allowed_update_types) {
    case AllowedUpdateTypes::kFullAndIncremental:
      if (!update_interval.count() || !full_update_interval.count()) {
        throw std::logic_error(
            fmt::format("Both {} and {} must be set for cache '{}'",
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
            "{} config field must only be used with full-and-incremental "
            "updated cache '{}'. Please rename it to {}.",
            kFullUpdateInterval, config.Name(), kUpdateInterval));
      }
      if (!update_interval.count()) {
        throw std::logic_error(fmt::format("{} is not set for cache '{}'",
                                           kUpdateInterval, config.Name()));
      }
      full_update_interval = update_interval;
      break;
  }

  if (max_dump_age && *max_dump_age <= std::chrono::milliseconds::zero()) {
    throw std::logic_error(fmt::format("{} must be positive for cache '{}'",
                                       kMaxDumpAge, config.Name()));
  }
  if (max_dump_count == 0) {
    throw std::logic_error(fmt::format("{} must not be 0 for cache '{}'",
                                       kMaxDumpCount, config.Name()));
  }
  if (dumps_enabled && !config[kDump].HasMember(kWorldReadable)) {
    throw std::logic_error(fmt::format(
        "If dumps are enabled, then '{}' must be set for cache '{}'",
        kWorldReadable, config.Name()));
  }
}

CacheConfigStatic CacheConfigStatic::MergeWith(const CacheConfig& other) const {
  CacheConfigStatic copy = *this;
  static_cast<CacheConfig&>(copy) = other;
  return copy;
}

LruCacheConfig::LruCacheConfig(const components::ComponentConfig& config)
    : size(config[kSize].As<size_t>()),
      lifetime(config[kLifetime].As<std::chrono::milliseconds>(0)),
      background_update(config[kBackgroundUpdate].As<bool>(false)
                            ? BackgroundUpdateMode::kEnabled
                            : BackgroundUpdateMode::kDisabled) {
  if (size == 0) throw std::runtime_error("cache-size is non-positive");
}

LruCacheConfig::LruCacheConfig(const formats::json::Value& value)
    : size(value[kSize].As<size_t>()),
      lifetime(ParseMs(value[kLifetimeMs])),
      background_update(value[kBackgroundUpdate].As<bool>(false)
                            ? BackgroundUpdateMode::kEnabled
                            : BackgroundUpdateMode::kDisabled) {
  if (size == 0) throw std::runtime_error("cache-size is non-positive");
}

LruCacheConfigStatic::LruCacheConfigStatic(
    const components::ComponentConfig& component_config)
    : config(component_config), ways(component_config[kWays].As<size_t>()) {
  if (ways <= 0) throw std::runtime_error("cache-ways is non-positive");
}

size_t LruCacheConfigStatic::GetWaySize() const {
  auto way_size = config.size / ways;
  return way_size == 0 ? 1 : way_size;
}

LruCacheConfigStatic LruCacheConfigStatic::MergeWith(
    const LruCacheConfig& other) const {
  LruCacheConfigStatic copy = *this;
  copy.config = other;
  return copy;
}

CacheConfigSet::CacheConfigSet(const taxi_config::DocsMap& docs_map) {
  UASSERT(!ConfigName().empty() || !LruConfigName().empty());

  const auto& config_name = ConfigName();
  if (!config_name.empty()) {
    auto caches_json = docs_map.Get(config_name);
    for (const auto& [name, value] : Items(caches_json)) {
      configs_.emplace(name, CacheConfig{value});
    }
  }

  const auto& lru_config_name = LruConfigName();
  if (!lru_config_name.empty()) {
    auto lru_caches_json = docs_map.Get(lru_config_name);
    for (const auto& [name, value] : Items(lru_caches_json)) {
      lru_configs_.emplace(name, LruCacheConfig{value});
    }
  }
}

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
