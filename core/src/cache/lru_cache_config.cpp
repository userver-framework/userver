#include <userver/cache/lru_cache_config.hpp>

#include <stdexcept>

#include <userver/components/component_config.hpp>
#include <userver/dump/config.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {

namespace {

constexpr std::string_view kWays = "ways";
constexpr std::string_view kSize = "size";
constexpr std::string_view kLifetime = "lifetime";
constexpr std::string_view kBackgroundUpdate = "background-update";
constexpr std::string_view kLifetimeMs = "lifetime-ms";

}  // namespace

using dump::impl::ParseMs;

LruCacheConfig::LruCacheConfig(const yaml_config::YamlConfig& config)
    : size(config[kSize].As<std::size_t>()),
      lifetime(config[kLifetime].As<std::chrono::milliseconds>(0)),
      background_update(config[kBackgroundUpdate].As<bool>(false)
                            ? BackgroundUpdateMode::kEnabled
                            : BackgroundUpdateMode::kDisabled) {
  if (size == 0) throw std::runtime_error("cache-size is non-positive");
}

LruCacheConfig::LruCacheConfig(const components::ComponentConfig& config)
    : LruCacheConfig(static_cast<const yaml_config::YamlConfig&>(config)) {}

LruCacheConfig::LruCacheConfig(const formats::json::Value& value)
    : size(value[kSize].As<std::size_t>()),
      lifetime(ParseMs(value[kLifetimeMs])),
      background_update(value[kBackgroundUpdate].As<bool>(false)
                            ? BackgroundUpdateMode::kEnabled
                            : BackgroundUpdateMode::kDisabled) {
  if (size == 0) throw std::runtime_error("cache-size is non-positive");
}

std::size_t LruCacheConfig::GetWaySize(std::size_t ways) const {
  const auto way_size = size / ways;
  return way_size == 0 ? 1 : way_size;
}

LruCacheConfig Parse(const formats::json::Value& value,
                     formats::parse::To<LruCacheConfig>) {
  return LruCacheConfig{value};
}

LruCacheConfigStatic::LruCacheConfigStatic(
    const yaml_config::YamlConfig& config)
    : config(config),
      ways(config[kWays].As<std::size_t>()),
      use_dynamic_config(config["config-settings"].As<bool>(true)) {
  if (ways <= 0) throw std::runtime_error("cache-ways is non-positive");
}

LruCacheConfigStatic::LruCacheConfigStatic(
    const components::ComponentConfig& config)
    : LruCacheConfigStatic(
          static_cast<const yaml_config::YamlConfig&>(config)) {}

std::size_t LruCacheConfigStatic::GetWaySize() const {
  return config.GetWaySize(ways);
}

std::unordered_map<std::string, LruCacheConfig> ParseLruCacheConfigSet(
    const dynamic_config::DocsMap& docs_map) {
  return docs_map.Get("USERVER_LRU_CACHES")
      .As<std::unordered_map<std::string, LruCacheConfig>>();
}

std::optional<LruCacheConfig> GetLruConfig(
    const dynamic_config::Snapshot& config, const std::string& cache_name) {
  return utils::FindOptional(config[kLruCacheConfigSet], cache_name);
}

}  // namespace cache

USERVER_NAMESPACE_END
