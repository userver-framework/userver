#pragma once

#include <chrono>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <userver/cache/update_type.hpp>
#include <userver/formats/json_fwd.hpp>
#include <userver/taxi_config/snapshot.hpp>
#include <userver/yaml_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {
struct Config;
}  // namespace dump

namespace cache {

class ConfigError : public std::logic_error {
 public:
  using std::logic_error::logic_error;
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
  std::optional<std::chrono::milliseconds> exception_interval;
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
  std::optional<std::string> task_processor_name;
  std::chrono::milliseconds cleanup_interval;
  bool is_strong_period;

  FirstUpdateMode first_update_mode;
  FirstUpdateType first_update_type;

  std::chrono::milliseconds update_interval;
  std::chrono::milliseconds update_jitter;
  std::chrono::milliseconds full_update_interval;
  std::optional<std::chrono::milliseconds> exception_interval;
  bool updates_enabled;
};

std::unordered_map<std::string, ConfigPatch> ParseCacheConfigSet(
    const dynamic_config::DocsMap& docs_map);

inline constexpr dynamic_config::Key<ParseCacheConfigSet> kCacheConfigSet;

}  // namespace cache

USERVER_NAMESPACE_END
