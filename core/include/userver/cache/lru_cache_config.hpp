#pragma once

#include <chrono>
#include <optional>
#include <unordered_map>

#include <userver/components/component_fwd.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/taxi_config/snapshot.hpp>

USERVER_NAMESPACE_BEGIN

namespace yaml_config {
class YamlConfig;
}  // namespace yaml_config

namespace cache {

namespace impl {
bool GetLruConfigSettings(const components::ComponentConfig& config);
}

enum class BackgroundUpdateMode {
  kEnabled,
  kDisabled,
};

struct LruCacheConfig final {
  explicit LruCacheConfig(const yaml_config::YamlConfig& config);
  explicit LruCacheConfig(const components::ComponentConfig& config);

  explicit LruCacheConfig(const formats::json::Value& value);

  size_t size;
  std::chrono::milliseconds lifetime;
  BackgroundUpdateMode background_update;
};

LruCacheConfig Parse(const formats::json::Value& value,
                     formats::parse::To<LruCacheConfig>);

struct LruCacheConfigStatic final {
  explicit LruCacheConfigStatic(const yaml_config::YamlConfig& config);
  explicit LruCacheConfigStatic(const components::ComponentConfig& config);

  LruCacheConfigStatic MergeWith(const LruCacheConfig& other) const;

  size_t GetWaySize() const;

  LruCacheConfig config;
  size_t ways;
};

std::unordered_map<std::string, LruCacheConfig> ParseLruCacheConfigSet(
    const taxi_config::DocsMap& docs_map);

inline constexpr taxi_config::Key<ParseLruCacheConfigSet> kLruCacheConfigSet;

std::optional<LruCacheConfig> GetLruConfig(const taxi_config::Snapshot& config,
                                           const std::string& cache_name);

}  // namespace cache

USERVER_NAMESPACE_END
