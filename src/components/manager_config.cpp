#include "manager_config.hpp"

#include <fstream>

#include <json/reader.h>

#include <json_config/value.hpp>

namespace components {

namespace {

template <typename T>
ManagerConfig ParseFromAny(T&& source, const std::string& source_desc) {
  static const std::string kConfigVarsField = "config_vars";
  static const std::string kManagerConfigField = "components_manager";

  Json::Reader reader;
  Json::Value config_json;
  if (!reader.parse(source, config_json)) {
    throw std::runtime_error("Cannot parse config from '" + source_desc +
                             "': " + reader.getFormattedErrorMessages());
  }

  auto config_vars_path = json_config::ParseOptionalString(
      config_json, kConfigVarsField, kConfigVarsField, {});
  json_config::VariableMapPtr config_vars_ptr;
  if (config_vars_path) {
    config_vars_ptr = std::make_shared<json_config::VariableMap>(
        json_config::VariableMap::ParseFromFile(*config_vars_path));
  }
  return ManagerConfig::ParseFromJson(
      std::move(config_json), kManagerConfigField, std::move(config_vars_ptr));
}

}  // namespace

ManagerConfig ManagerConfig::ParseFromJson(
    Json::Value json, const std::string& name,
    json_config::VariableMapPtr config_vars_ptr) {
  ManagerConfig config;
  config.json = std::move(json);
  config.config_vars_ptr = config_vars_ptr;

  const auto& value = config.json[name];

  config.coro_pool = engine::coro::PoolConfig::ParseFromJson(
      value["coro_pool"], name + ".coro_pool", config_vars_ptr);
  config.event_thread_pool = engine::ev::ThreadPoolConfig::ParseFromJson(
      value["event_thread_pool"], name + ".event_thread_pool", config_vars_ptr);
  config.components = json_config::ParseArray<components::ComponentConfig>(
      value, "components", name, config_vars_ptr);
  config.task_processors = json_config::ParseArray<engine::TaskProcessorConfig>(
      value, "task_processors", name, config_vars_ptr);
  config.default_task_processor = json_config::ParseString(
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
