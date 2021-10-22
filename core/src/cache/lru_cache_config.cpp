#include <userver/cache/lru_cache_config.hpp>

#include <stdexcept>

#include <userver/dump/config.hpp>
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

LruCacheConfig Parse(const formats::json::Value& value,
                     formats::parse::To<LruCacheConfig>) {
  return LruCacheConfig{value};
}

LruCacheConfigStatic::LruCacheConfigStatic(
    const yaml_config::YamlConfig& config)
    : config(config), ways(config[kWays].As<size_t>()) {
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

std::unordered_map<std::string, LruCacheConfig> ParseLruCacheConfigSet(
    const taxi_config::DocsMap& docs_map) {
  return docs_map.Get("USERVER_LRU_CACHES")
      .As<std::unordered_map<std::string, LruCacheConfig>>();
}

std::optional<LruCacheConfig> GetLruConfig(const taxi_config::Snapshot& config,
                                           const std::string& cache_name) {
  return utils::FindOptional(config[kLruCacheConfigSet], cache_name);
}

}  // namespace cache

USERVER_NAMESPACE_END
