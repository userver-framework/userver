#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <dynamic_config/variables/USERVER_DUMPS.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

namespace impl {
std::chrono::milliseconds
ParseMs(const formats::json::Value& value, std::optional<std::chrono::milliseconds> default_value = {});
}

extern const std::string_view kDump;
extern const std::string_view kMaxDumpAge;
extern const std::string_view kMinDumpInterval;

using ConfigPatch = ::dynamic_config::userver_dumps::ConfigPatch;

struct Config final {
    Config(std::string name, const yaml_config::YamlConfig& config, std::string_view dump_root);

    std::string name;
    uint64_t dump_format_version;
    bool world_readable;
    std::string dump_directory;
    std::optional<std::string> fs_task_processor;
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

}  // namespace dump

USERVER_NAMESPACE_END
