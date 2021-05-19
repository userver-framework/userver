#include <dump/config.hpp>

#include <utility>

#include <fmt/format.h>

#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <components/dump_configurator.hpp>
#include <utils/algo.hpp>

namespace dump {

namespace {

constexpr std::string_view kDumpsEnabled = "enable";
constexpr std::string_view kMinDumpInterval = "min-interval";
constexpr std::string_view kFsTaskProcessor = "fs-task-processor";
constexpr std::string_view kDumpFormatVersion = "format-version";
constexpr std::string_view kMaxDumpCount = "max-count";
constexpr std::string_view kWorldReadable = "world-readable";
constexpr std::string_view kEncrypted = "encrypted";

constexpr auto kDefaultFsTaskProcessor = std::string_view{"fs-task-processor"};
constexpr auto kDefaultMaxDumpCount = uint64_t{1};

}  // namespace

namespace impl {

std::chrono::milliseconds ParseMs(
    const formats::json::Value& value,
    std::optional<std::chrono::milliseconds> default_value) {
  const auto result = std::chrono::milliseconds{
      default_value ? value.As<int64_t>(default_value->count())
                    : value.As<int64_t>()};
  if (result < std::chrono::milliseconds::zero()) {
    throw formats::json::ParseException("Negative duration");
  }
  return result;
}

}  // namespace impl

ConfigPatch Parse(const formats::json::Value& value,
                  formats::parse::To<ConfigPatch>) {
  const auto min_dump_interval = value["min-dump-interval-ms"];
  return {value["dumps-enabled"].As<std::optional<bool>>(),
          min_dump_interval.IsMissing()
              ? std::nullopt
              : std::optional{impl::ParseMs(min_dump_interval, {{}})}};
}

Config::Config(std::string name, const yaml_config::YamlConfig& config,
               std::string_view dump_root)
    : name(std::move(name)),
      dump_format_version(config[kDumpFormatVersion].As<uint64_t>()),
      world_readable(config[kWorldReadable].As<bool>()),
      dump_directory(fmt::format("{}/{}", dump_root, this->name)),
      fs_task_processor(
          config[kFsTaskProcessor].As<std::string>(kDefaultFsTaskProcessor)),
      max_dump_count(config[kMaxDumpCount].As<uint64_t>(kDefaultMaxDumpCount)),
      max_dump_age(
          config[kMaxDumpAge].As<std::optional<std::chrono::milliseconds>>()),
      max_dump_age_set(config.HasMember(kMaxDumpAge)),
      dump_is_encrypted(config[kEncrypted].As<bool>(false)),
      dumps_enabled(config[kDumpsEnabled].As<bool>()),
      min_dump_interval(
          config[kMinDumpInterval].As<std::chrono::milliseconds>(0)) {
  if (max_dump_age && *max_dump_age <= std::chrono::milliseconds::zero()) {
    throw std::logic_error(
        fmt::format("{}: {} must be positive", this->name, kMaxDumpAge));
  }
  if (max_dump_count == 0) {
    throw std::logic_error(
        fmt::format("{}: {} must not be 0", this->name, kMaxDumpCount));
  }
}

std::optional<Config> Config::ParseOptional(
    const components::ComponentConfig& config,
    const components::ComponentContext& context) {
  if (!config.HasMember(kDump)) return {};
  return Config{
      config.Name(), config[kDump],
      context.FindComponent<components::DumpConfigurator>().GetDumpRoot()};
}

Config Config::MergeWith(const ConfigPatch& patch) const {
  Config copy = *this;
  copy.dumps_enabled = patch.dumps_enabled.value_or(copy.dumps_enabled);
  copy.min_dump_interval =
      patch.min_dump_interval.value_or(copy.min_dump_interval);
  return copy;
}

ConfigSet::ConfigSet(const taxi_config::DocsMap& docs_map)
    : configs_(docs_map.Get("USERVER_DUMPS")
                   .As<std::unordered_map<std::string, ConfigPatch>>()) {}

std::optional<ConfigPatch> ConfigSet::GetConfig(
    const std::string& component_name) const {
  return utils::FindOptional(configs_, component_name);
}

}  // namespace dump
