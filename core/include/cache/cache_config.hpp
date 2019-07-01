#pragma once

#include <chrono>
#include <unordered_map>

#include <boost/optional.hpp>

#include <components/component_config.hpp>
#include <taxi_config/config.hpp>
#include <taxi_config/value.hpp>

namespace cache {

enum class AllowedUpdateTypes {
  kFullAndIncremental,
  kOnlyFull,
};

struct CacheConfig {
  explicit CacheConfig(const components::ComponentConfig& config);

  CacheConfig(std::chrono::milliseconds update_interval,
              std::chrono::milliseconds update_jitter,
              std::chrono::milliseconds full_update_interval);

  AllowedUpdateTypes allowed_update_types;
  std::chrono::milliseconds update_interval;
  std::chrono::milliseconds update_jitter;
  std::chrono::milliseconds full_update_interval;
};

struct LruCacheConfig {
  LruCacheConfig(size_t size, std::chrono::milliseconds lifetime);

  size_t size;
  std::chrono::milliseconds lifetime;
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
  boost::optional<CacheConfig> GetConfig(const std::string& cache_name) const;

  /// Get config for LRU cache
  boost::optional<LruCacheConfig> GetLruConfig(
      const std::string& cache_name) const;

  static bool IsConfigEnabled();

  static bool IsLruConfigEnabled();

  static void SetConfigName(const std::string& name);

  static void SetLruConfigName(const std::string& name);

 private:
  static CacheConfig ParseConfig(const formats::json::Value& json);

  static LruCacheConfig ParseLruConfig(const formats::json::Value& json);

  static std::string& LruConfigName();

  static std::string& ConfigName();

 private:
  std::unordered_map<std::string, CacheConfig> configs_;
  std::unordered_map<std::string, LruCacheConfig> lru_configs_;
};

void CacheConfigInit();

}  // namespace cache
