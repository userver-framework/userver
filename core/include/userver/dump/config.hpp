#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

namespace impl {
std::chrono::milliseconds ParseMs(
    const formats::json::Value& value,
    std::optional<std::chrono::milliseconds> default_value = {});
}

extern const std::string_view kDump;
extern const std::string_view kMaxDumpAge;
extern const std::string_view kMinDumpInterval;

struct ConfigPatch final {
  std::optional<bool> dumps_enabled;
  std::optional<std::chrono::milliseconds> min_dump_interval;
};

ConfigPatch Parse(const formats::json::Value& value,
                  formats::parse::To<ConfigPatch>);

struct Config final {
  Config(std::string name, const yaml_config::YamlConfig& config,
         std::string_view dump_root);

  std::string name;
  uint64_t dump_format_version;
  bool world_readable;
  std::string dump_directory;
  std::string fs_task_processor;
  uint64_t max_dump_count;
  std::optional<std::chrono::milliseconds> max_dump_age;
  bool max_dump_age_set;
  bool dump_is_encrypted;

  bool static_dumps_enabled;
  std::chrono::milliseconds static_min_dump_interval;
};

struct DynamicConfig final {
  explicit DynamicConfig(const Config& config, ConfigPatch&& patch);

  bool operator==(const DynamicConfig& other) const noexcept;
  bool operator!=(const DynamicConfig& other) const noexcept;

  bool dumps_enabled;
  std::chrono::milliseconds min_dump_interval;
};

std::unordered_map<std::string, ConfigPatch> ParseConfigSet(
    const dynamic_config::DocsMap& docs_map);

inline constexpr dynamic_config::Key<ParseConfigSet> kConfigSet;

}  // namespace dump

USERVER_NAMESPACE_END
