#include <cache/dump/config.hpp>

#include <fmt/format.h>

#include <utils/algo.hpp>

namespace cache::dump {

namespace {

constexpr std::string_view kDumpsEnabled = "enable";
constexpr std::string_view kDumpDirectory = "userver-cache-dump-path";
constexpr std::string_view kMinDumpInterval = "min-interval";
constexpr std::string_view kFsTaskProcessor = "fs-task-processor";
constexpr std::string_view kDumpFormatVersion = "format-version";
constexpr std::string_view kMaxDumpCount = "max-count";
constexpr std::string_view kWorldReadable = "world-readable";
constexpr std::string_view kEncrypted = "encrypted";

constexpr auto kDefaultFsTaskProcessor = std::string_view{"fs-task-processor"};
constexpr auto kDefaultMaxDumpCount = uint64_t{1};

std::string ParseDumpDirectory(const components::ComponentConfig& config) {
  const auto dump_root = config.ConfigVars()[kDumpDirectory].As<std::string>();
  return fmt::format("{}/{}", dump_root, config.Name());
}

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
  return {value["dumps-enabled"].As<bool>(),
          impl::ParseMs(value["min-dump-interval"])};
}

Config::Config(const components::ComponentConfig& config)
    : name(config.Name()),
      dump_format_version(config[kDump][kDumpFormatVersion].As<uint64_t>()),
      world_readable(config[kDump][kWorldReadable].As<bool>()),
      dump_directory(ParseDumpDirectory(config)),
      fs_task_processor(config[kDump][kFsTaskProcessor].As<std::string>(
          kDefaultFsTaskProcessor)),
      max_dump_count(
          config[kDump][kMaxDumpCount].As<uint64_t>(kDefaultMaxDumpCount)),
      max_dump_age(config[kDump][kMaxDumpAge]
                       .As<std::optional<std::chrono::milliseconds>>()),
      max_dump_age_set(config[kDump].HasMember(kMaxDumpAge)),
      dump_is_encrypted(config[kDump][kEncrypted].As<bool>(false)),
      dumps_enabled(config[kDump][kDumpsEnabled].As<bool>()),
      min_dump_interval(
          config[kDump][kMinDumpInterval].As<std::chrono::milliseconds>(0)) {
  if (max_dump_age && *max_dump_age <= std::chrono::milliseconds::zero()) {
    throw std::logic_error(fmt::format("{} must be positive for cache '{}'",
                                       kMaxDumpAge, config.Name()));
  }
  if (max_dump_count == 0) {
    throw std::logic_error(fmt::format("{} must not be 0 for cache '{}'",
                                       kMaxDumpCount, config.Name()));
  }
  for (const auto required_key : {kWorldReadable, kDumpFormatVersion}) {
    if (!config[kDump].HasMember(required_key)) {
      throw std::logic_error(fmt::format(
          "If dumps are enabled, then '{}' must be set for cache '{}'",
          required_key, config.Name()));
    }
  }
}

std::optional<Config> Config::ParseOptional(
    const components::ComponentConfig& config) {
  return config.HasMember(kDump) ? std::optional<Config>(std::in_place, config)
                                 : std::nullopt;
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

}  // namespace cache::dump
