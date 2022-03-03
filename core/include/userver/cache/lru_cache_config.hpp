#pragma once

#include <chrono>
#include <optional>
#include <unordered_map>

#include <userver/components/component_fwd.hpp>
#include <userver/formats/json_fwd.hpp>
#include <userver/taxi_config/snapshot.hpp>
#include <userver/yaml_config/fwd.hpp>

// TODO remove extra includes
#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

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
    const dynamic_config::DocsMap& docs_map);

inline constexpr dynamic_config::Key<ParseLruCacheConfigSet> kLruCacheConfigSet;

std::optional<LruCacheConfig> GetLruConfig(
    const dynamic_config::Snapshot& config, const std::string& cache_name);

}  // namespace cache

USERVER_NAMESPACE_END
