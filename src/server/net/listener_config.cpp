#include "listener_config.hpp"

#include <stdexcept>
#include <string>

#include <json_config/value.hpp>

namespace server {
namespace net {

namespace {

int CheckPort(int port, const std::string& name) {
  if (port <= 0 || port >= 65536) {
    throw std::runtime_error("Invalid " + name);
  }
  return port;
}

}  // namespace

ListenerConfig ListenerConfig::ParseFromJson(
    const Json::Value& json, const std::string& full_path,
    const json_config::VariableMapPtr& config_vars_ptr) {
  ListenerConfig config;
  config.connection_config = ConnectionConfig::ParseFromJson(
      json["connection"], full_path + ".connection", config_vars_ptr);

  auto optional_port =
      json_config::ParseOptionalInt(json, "port", full_path, config_vars_ptr);
  if (optional_port)
    config.port = CheckPort(*optional_port, full_path + ".port");
  auto optional_monitor_port = json_config::ParseOptionalInt(
      json, "monitor_port", full_path, config_vars_ptr);
  if (optional_monitor_port)
    config.monitor_port =
        CheckPort(*optional_monitor_port, full_path + ".monitor_port");
  if (config.port == config.monitor_port) {
    throw std::runtime_error("Same port used for requests and monitoring in " +
                             full_path);
  }

  auto optional_backlog = json_config::ParseOptionalInt(
      json, "backlog", full_path, config_vars_ptr);
  if (optional_backlog) config.backlog = *optional_backlog;
  if (config.backlog <= 0) {
    throw std::runtime_error("Invalid backlog value in " + full_path);
  }

  return config;
}

}  // namespace net
}  // namespace server
