#pragma once

#include <chrono>
#include <cstddef>
#include <optional>
#include <unordered_map>

#include <userver/components/component_fwd.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/formats/json_fwd.hpp>
#include <userver/yaml_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {

enum class BackgroundUpdateMode {
  kEnabled,
  kDisabled,
};

struct LruCacheConfig final {
  explicit LruCacheConfig(const yaml_config::YamlConfig& config);
  explicit LruCacheConfig(const components::ComponentConfig& config);

  explicit LruCacheConfig(const formats::json::Value& value);

  std::size_t GetWaySize(std::size_t ways) const;

  std::size_t size;
  std::chrono::milliseconds lifetime;
  BackgroundUpdateMode background_update;
};

LruCacheConfig Parse(const formats::json::Value& value,
                     formats::parse::To<LruCacheConfig>);

struct LruCacheConfigStatic final {
  explicit LruCacheConfigStatic(const yaml_config::YamlConfig& config);
  explicit LruCacheConfigStatic(const components::ComponentConfig& config);

  std::size_t GetWaySize() const;

  LruCacheConfig config;
  std::size_t ways;
  bool use_dynamic_config;
};

std::unordered_map<std::string, LruCacheConfig> ParseLruCacheConfigSet(
    const dynamic_config::DocsMap& docs_map);

inline constexpr dynamic_config::Key<ParseLruCacheConfigSet> kLruCacheConfigSet;

std::optional<LruCacheConfig> GetLruConfig(
    const dynamic_config::Snapshot& config, const std::string& cache_name);

}  // namespace cache

USERVER_NAMESPACE_END
