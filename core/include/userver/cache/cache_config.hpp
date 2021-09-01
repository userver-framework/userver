#pragma once

#include <chrono>
#include <optional>
#include <unordered_map>

#include <userver/cache/lru_cache_config.hpp>  // TODO remove the include
#include <userver/formats/json/value.hpp>
#include <userver/taxi_config/snapshot.hpp>

namespace yaml_config {
class YamlConfig;
}  // namespace yaml_config

namespace dump {
struct Config;
}  // namespace dump

namespace cache {

class ConfigError : public std::logic_error {
 public:
  using std::logic_error::logic_error;
};

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

enum class FirstUpdateType {
  kFull,
  kIncremental,
  kIncrementalThenAsyncFull,
};

struct ConfigPatch final {
  std::chrono::milliseconds update_interval;
  std::chrono::milliseconds update_jitter;
  std::chrono::milliseconds full_update_interval;
  bool updates_enabled;
};

ConfigPatch Parse(const formats::json::Value& value,
                  formats::parse::To<ConfigPatch>);

struct Config final {
  explicit Config(const yaml_config::YamlConfig& config,
                  const std::optional<dump::Config>& dump_config);

  Config MergeWith(const ConfigPatch& patch) const;

  AllowedUpdateTypes allowed_update_types;
  bool allow_first_update_failure;
  std::optional<bool> force_periodic_update;
  bool config_updates_enabled;
  std::chrono::milliseconds cleanup_interval;

  FirstUpdateMode first_update_mode;
  FirstUpdateType first_update_type;

  std::chrono::milliseconds update_interval;
  std::chrono::milliseconds update_jitter;
  std::chrono::milliseconds full_update_interval;
  bool updates_enabled;
};

std::unordered_map<std::string, ConfigPatch> ParseCacheConfigSet(
    const taxi_config::DocsMap& docs_map);

inline constexpr taxi_config::Key<ParseCacheConfigSet> kCacheConfigSet;

}  // namespace cache
