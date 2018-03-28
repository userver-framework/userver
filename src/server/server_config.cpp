#include "server_config.hpp"

#include <fstream>

#include <json/reader.h>

#include <json_config/value.hpp>
#include <json_config/variable_map.hpp>

namespace server {

namespace {

template <typename T>
ServerConfig ParseFromAny(T&& source, const std::string& source_desc) {
  static const std::string kConfigVarsField = "config_vars";
  static const std::string kServerConfigField = "server";

  Json::Reader reader;
  Json::Value config_json;
  if (!reader.parse(source, config_json)) {
    throw std::runtime_error("Cannot parse server config from '" + source_desc +
                             "': " + reader.getFormattedErrorMessages());
  }

  auto config_vars_path = json_config::ParseOptionalString(
      config_json, kConfigVarsField, kConfigVarsField, {});
  json_config::VariableMapPtr config_vars_ptr;
  if (config_vars_path) {
    config_vars_ptr = std::make_shared<json_config::VariableMap>(
        json_config::VariableMap::ParseFromFile(*config_vars_path));
  }
  return ServerConfig::ParseFromJson(std::move(config_json), kServerConfigField,
                                     std::move(config_vars_ptr));
}

}  // namespace

ServerConfig ServerConfig::ParseFromJson(
    Json::Value json, const std::string& name,
    json_config::VariableMapPtr config_vars_ptr) {
  ServerConfig config;
  config.json = std::move(json);
  config.config_vars_ptr = config_vars_ptr;

  const auto& value = config.json[name];

  config.listener = net::ListenerConfig::ParseFromJson(
      value["listener"], name + ".listener", config_vars_ptr);
  config.coro_pool = engine::coro::PoolConfig::ParseFromJson(
      value["coro_pool"], name + ".coro_pool", config_vars_ptr);
  config.logger_access = json_config::ParseOptionalString(
      value, "logger_access", name, config_vars_ptr);
  config.logger_access_tskv = json_config::ParseOptionalString(
      value, "logger_access_tskv", name, config_vars_ptr);
  config.schedulers = json_config::ParseArray<SchedulerConfig>(
      value, "schedulers", name, config_vars_ptr);
  config.components = json_config::ParseArray<components::ComponentConfig>(
      value, "components", name, config_vars_ptr);
  config.task_processors = json_config::ParseArray<engine::TaskProcessorConfig>(
      value, "task_processors", name, config_vars_ptr);
  config.default_task_processor = json_config::ParseString(
      value, "default_task_processor", name, config_vars_ptr);
  return config;
}

ServerConfig ServerConfig::ParseFromString(const std::string& str) {
  return ParseFromAny(str, "<std::string>");
}

ServerConfig ServerConfig::ParseFromFile(const std::string& path) {
  std::ifstream input_stream(path);
  if (!input_stream) {
    throw std::runtime_error("Cannot open server config file '" + path + '\'');
  }
  return ParseFromAny(input_stream, path);
}

}  // namespace server
