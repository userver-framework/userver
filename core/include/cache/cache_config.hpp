#pragma once

#include <chrono>
#include <unordered_map>

#include <boost/optional.hpp>

#include <components/component_config.hpp>
#include <taxi_config/config.hpp>
#include <taxi_config/value.hpp>

namespace components {

struct CacheConfig {
  explicit CacheConfig(const ComponentConfig& config);
  CacheConfig(std::chrono::milliseconds update_interval,
              std::chrono::milliseconds update_jitter,
              std::chrono::milliseconds full_update_interval);

  std::chrono::milliseconds update_interval_;
  std::chrono::milliseconds update_jitter_;
  std::chrono::milliseconds full_update_interval_;
};

class CacheConfigSet {
 public:
  explicit CacheConfigSet(const taxi_config::DocsMap& docs_map);

  /// Get config for cache
  boost::optional<CacheConfig> GetConfig(const std::string& cache_name) const;

  static bool IsConfigEnabled();

  static void SetConfigName(const std::string& name);

 private:
  static CacheConfig ParseConfig(formats::json::Value json);

  static std::string& ConfigName();

 private:
  std::unordered_map<std::string, CacheConfig> configs_;
};

void CacheConfigInit();

}  // namespace components
