#pragma once

#include <storages/postgres/postgres_fwd.hpp>
#include <taxi_config/value.hpp>

namespace storages {
namespace postgres {

CommandControl ParseJson(const formats::json::Value& elem,
                         const CommandControl*);

class Config {
 public:
  taxi_config::Value<CommandControl> default_command_control;

  Config(const taxi_config::DocsMap& docs_map)
      : default_command_control{"POSTGRES_DEFAULT_COMMAND_CONTROL", docs_map} {}
};

}  // namespace postgres
}  // namespace storages
