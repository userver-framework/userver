#include <components/manager_config.hpp>

#include <fstream>

#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/formats/yaml/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/yaml_config/map_to_array.hpp>
#include <utils/userver_experiment.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

formats::yaml::Value ParseYaml(const std::string& doc) {
  return formats::yaml::FromString(doc);
}

formats::yaml::Value ParseYaml(std::istream& is) {
  return formats::yaml::FromStream(is);
}

template <typename T>
ManagerConfig ParseFromAny(
    T&& source, const std::string& source_desc,
    const std::optional<std::string>& user_config_vars_path,
    const std::optional<std::string>& user_config_vars_override_path) {
  static const std::string kConfigVarsField = "config_vars";
  static const std::string kConfigVarsLocalDirField = "config_vars_local";
  static const std::string kManagerConfigField = "components_manager";
  static const std::string kUserverExperimentsField = "userver_experiments";

  formats::yaml::Value config_yaml;
  try {
    config_yaml = ParseYaml(source);
  } catch (const formats::yaml::Exception& e) {
    throw std::runtime_error("Cannot parse config from '" + source_desc +
                             "': " + e.what());
  }

  std::optional<std::string> config_vars_path = user_config_vars_path;
  if (!config_vars_path) {
    config_vars_path =
        config_yaml[kConfigVarsField].As<std::optional<std::string>>();
  }
  formats::yaml::Value config_vars;
  if (config_vars_path) {
    config_vars = formats::yaml::blocking::FromFile(*config_vars_path);
  }

  if (user_config_vars_override_path) {
    formats::yaml::ValueBuilder builder = config_vars;

    auto local_config_vars =
        formats::yaml::blocking::FromFile(*user_config_vars_override_path);
    for (const auto& [name, value] : Items(local_config_vars)) {
      builder[name] = value;
    }
    config_vars = builder.ExtractValue();
  }

  utils::ParseUserverExperiments(config_yaml[kUserverExperimentsField]);

  return yaml_config::YamlConfig(config_yaml[kManagerConfigField],
                                 std::move(config_vars))
      .As<ManagerConfig>();
}

}  // namespace

ManagerConfig Parse(const yaml_config::YamlConfig& value,
                    formats::parse::To<ManagerConfig>) {
  ManagerConfig config;
  config.source = value;

  config.coro_pool = value["coro_pool"].As<engine::coro::PoolConfig>();
  config.event_thread_pool =
      value["event_thread_pool"].As<engine::ev::ThreadPoolConfig>();
  if (config.event_thread_pool.threads < 1) {
    throw std::runtime_error(
        "In static config the components_manager.event_thread_pool.threads "
        "must be greater than 0");
  }
  config.components = yaml_config::ParseMapToArray<components::ComponentConfig>(
      value["components"]);
  config.task_processors =
      yaml_config::ParseMapToArray<engine::TaskProcessorConfig>(
          value["task_processors"]);
  config.default_task_processor =
      value["default_task_processor"].As<std::string>();
  return config;
}

ManagerConfig ManagerConfig::FromString(
    const std::string& str, const std::optional<std::string>& config_vars_path,
    const std::optional<std::string>& config_vars_override_path) {
  return ParseFromAny(str, "<std::string>", config_vars_path,
                      config_vars_override_path);
}

ManagerConfig ManagerConfig::FromFile(
    const std::string& path, const std::optional<std::string>& config_vars_path,
    const std::optional<std::string>& config_vars_override_path) {
  std::ifstream input_stream(path);
  if (!input_stream) {
    throw std::runtime_error("Cannot open config file '" + path + '\'');
  }
  return ParseFromAny(input_stream, path, config_vars_path,
                      config_vars_override_path);
}

}  // namespace components

USERVER_NAMESPACE_END
