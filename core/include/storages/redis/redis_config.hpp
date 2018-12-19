#pragma once

#include <redis/base.hpp>
#include <taxi_config/value.hpp>

namespace redis {
CommandControl ParseJson(const formats::json::Value& elem, CommandControl*);
}

namespace storages {
namespace redis {

class Config {
 public:
  taxi_config::Value<::redis::CommandControl> default_command_control;

  Config(const taxi_config::DocsMap& docs_map)
      : default_command_control{"REDIS_DEFAULT_COMMAND_CONTROL", docs_map} {}
};

}  // namespace redis
}  // namespace storages
