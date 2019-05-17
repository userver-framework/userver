#pragma once

#include <storages/postgres/options.hpp>
#include <taxi_config/value.hpp>

namespace storages {
namespace postgres {

CommandControl Parse(const formats::json::Value& elem,
                     formats::parse::To<CommandControl>);

class Config {
 public:
  taxi_config::Value<CommandControl> default_command_control;

  Config(const taxi_config::DocsMap& docs_map)
      : default_command_control{"POSTGRES_DEFAULT_COMMAND_CONTROL", docs_map} {}
};

}  // namespace postgres
}  // namespace storages
