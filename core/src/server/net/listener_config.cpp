#include "listener_config.hpp"

#include <stdexcept>
#include <string>

#include <yaml_config/value.hpp>

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

ListenerConfig ListenerConfig::ParseFromYaml(
    const formats::yaml::Value& yaml, const std::string& full_path,
    const yaml_config::VariableMapPtr& config_vars_ptr) {
  ListenerConfig config;
  config.connection_config = ConnectionConfig::ParseFromYaml(
      yaml["connection"], full_path + ".connection", config_vars_ptr);

  auto optional_port =
      yaml_config::ParseOptionalInt(yaml, "port", full_path, config_vars_ptr);
  auto optional_unix = yaml_config::ParseOptionalString(
      yaml, "unix-socket", full_path, config_vars_ptr);
  if (optional_port && optional_unix)
    throw std::runtime_error(
        "Both 'port' and 'unix-socket' fields are set, only single field may "
        "be set at a time");
  if (!optional_port && !optional_unix)
    throw std::runtime_error(
        "Either 'port' or 'unix-socket' fields must be set");
  if (optional_port)
    config.port = CheckPort(*optional_port, full_path + ".port");
  if (optional_unix) config.unix_socket_path = *optional_unix;

  auto optional_backlog = yaml_config::ParseOptionalInt(
      yaml, "backlog", full_path, config_vars_ptr);
  if (optional_backlog) config.backlog = *optional_backlog;
  if (config.backlog <= 0) {
    throw std::runtime_error("Invalid backlog value in " + full_path);
  }

  auto optional_max_connections = yaml_config::ParseOptionalUint64(
      yaml, "max_connections", full_path, config_vars_ptr);
  if (optional_max_connections)
    config.max_connections = *optional_max_connections;

  config.shards = yaml_config::ParseOptionalUint64(yaml, "shards", full_path,
                                                   config_vars_ptr);
  config.task_processor = yaml_config::ParseString(yaml, "task_processor",
                                                   full_path, config_vars_ptr);
  return config;
}

}  // namespace net
}  // namespace server
