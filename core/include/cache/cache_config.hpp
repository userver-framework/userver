#pragma once

#include <chrono>
#include <optional>
#include <unordered_map>

#include <cache/dump/config.hpp>
#include <components/component_config.hpp>
#include <taxi_config/config.hpp>
#include <taxi_config/value.hpp>

namespace cache {

enum class AllowedUpdateTypes {
  kFullAndIncremental,
  kOnlyFull,
  kOnlyIncremental,
};

enum class FirstUpdateMode {
  kRequired,
  kBestEffort,
  kSkip,
};

struct CacheConfig {
  explicit CacheConfig(const components::ComponentConfig& config);

  explicit CacheConfig(const formats::json::Value& value);

  std::chrono::milliseconds update_interval;
  std::chrono::milliseconds update_jitter;
  std::chrono::milliseconds full_update_interval;
  std::chrono::milliseconds cleanup_interval;
};

struct CacheConfigStatic : public CacheConfig {
  explicit CacheConfigStatic(const components::ComponentConfig& config,
                             const std::optional<dump::Config>& dump_config);

  CacheConfigStatic MergeWith(const CacheConfig& other) const;

  AllowedUpdateTypes allowed_update_types;
  bool allow_first_update_failure;
  std::optional<bool> force_periodic_update;
  bool config_updates_enabled;

  FirstUpdateMode first_update_mode;
  bool force_full_second_update;
};

enum class BackgroundUpdateMode {
  kEnabled,
  kDisabled,
};

struct LruCacheConfig {
  explicit LruCacheConfig(const components::ComponentConfig& config);

  explicit LruCacheConfig(const formats::json::Value& value);

  size_t size;
  std::chrono::milliseconds lifetime;
  BackgroundUpdateMode background_update;
};

struct LruCacheConfigStatic {
  explicit LruCacheConfigStatic(const components::ComponentConfig& config);

  LruCacheConfigStatic MergeWith(const LruCacheConfig& other) const;

  size_t GetWaySize() const;

  LruCacheConfig config;
  size_t ways;
};

class CacheConfigSet final {
 public:
  explicit CacheConfigSet(const taxi_config::DocsMap& docs_map);

  /// Get config for cache
  std::optional<CacheConfig> GetConfig(const std::string& cache_name) const;

  /// Get config for LRU cache
  std::optional<LruCacheConfig> GetLruConfig(
      const std::string& cache_name) const;

  static bool IsConfigEnabled();

  static bool IsLruConfigEnabled();

  static void SetConfigName(const std::string& name);

  static void SetLruConfigName(const std::string& name);

 private:
  static std::string& LruConfigName();

  static std::string& ConfigName();

 private:
  std::unordered_map<std::string, CacheConfig> configs_;
  std::unordered_map<std::string, LruCacheConfig> lru_configs_;
};

}  // namespace cache
