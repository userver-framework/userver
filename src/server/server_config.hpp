#pragma once

#include <string>
#include <vector>

#include <json/value.h>
#include <boost/optional.hpp>

#include <components/component_config.hpp>
#include <engine/coro/pool_config.hpp>
#include <engine/task/task_processor_config.hpp>
#include <json_config/variable_map.hpp>
#include <server/net/listener.hpp>

#include "event_thread_pool_config.hpp"

namespace server {

struct ServerConfig {
  net::ListenerConfig listener;
  engine::coro::PoolConfig coro_pool;
  boost::optional<std::string> logger_access;
  boost::optional<std::string> logger_access_tskv;
  std::vector<EventThreadPoolConfig> event_thread_pools;
  std::vector<components::ComponentConfig> components;
  std::vector<engine::TaskProcessorConfig> task_processors;
  std::string default_task_processor;

  Json::Value json;  // the owner
  json_config::VariableMapPtr config_vars_ptr;

  static ServerConfig ParseFromJson(
      Json::Value json, const std::string& name,
      json_config::VariableMapPtr config_vars_ptr);

  static ServerConfig ParseFromString(const std::string&);
  static ServerConfig ParseFromFile(const std::string& path);
};

}  // namespace server
