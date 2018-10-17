#include <components/manager_config.hpp>

#include <fstream>

#include <yaml_config/value.hpp>

namespace components {

namespace {

template <typename T>
ManagerConfig ParseFromAny(T&& source, const std::string& source_desc) {
  static const std::string kConfigVarsField = "config_vars";
  static const std::string kManagerConfigField = "components_manager";

  formats::yaml::Node config_yaml;
  try {
    config_yaml = formats::yaml::Load(source);
  } catch (const formats::yaml::Exception& e) {
    throw std::runtime_error("Cannot parse config from '" + source_desc +
                             "': " + e.what());
  }

  auto config_vars_path = yaml_config::ParseOptionalString(
      config_yaml, kConfigVarsField, kConfigVarsField, {});
  yaml_config::VariableMapPtr config_vars_ptr;
  if (config_vars_path) {
    config_vars_ptr = std::make_shared<yaml_config::VariableMap>(
        yaml_config::VariableMap::ParseFromFile(*config_vars_path));
  }
  return ManagerConfig::ParseFromYaml(
      std::move(config_yaml), kManagerConfigField, std::move(config_vars_ptr));
}

}  // namespace

ManagerConfig ManagerConfig::ParseFromYaml(
    formats::yaml::Node yaml, const std::string& name,
    yaml_config::VariableMapPtr config_vars_ptr) {
  ManagerConfig config;
  config.yaml = std::move(yaml);
  config.config_vars_ptr = config_vars_ptr;

  const auto& value = config.yaml[name];

  config.coro_pool = engine::coro::PoolConfig::ParseFromYaml(
      value["coro_pool"], name + ".coro_pool", config_vars_ptr);
  config.event_thread_pool = engine::ev::ThreadPoolConfig::ParseFromYaml(
      value["event_thread_pool"], name + ".event_thread_pool", config_vars_ptr);
  config.components = yaml_config::ParseMapAsArray<components::ComponentConfig>(
      value, "components", name, config_vars_ptr);
  config.task_processors = yaml_config::ParseArray<engine::TaskProcessorConfig>(
      value, "task_processors", name, config_vars_ptr);
  config.default_task_processor = yaml_config::ParseString(
      value, "default_task_processor", name, config_vars_ptr);
  return config;
}

ManagerConfig ManagerConfig::ParseFromString(const std::string& str) {
  return ParseFromAny(str, "<std::string>");
}

ManagerConfig ManagerConfig::ParseFromFile(const std::string& path) {
  std::ifstream input_stream(path);
  if (!input_stream) {
    throw std::runtime_error("Cannot open config file '" + path + '\'');
  }
  return ParseFromAny(input_stream, path);
}

}  // namespace components
