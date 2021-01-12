#pragma once

#include <taxi_config/value.hpp>

#include <storages/postgres/options.hpp>

namespace storages {
namespace postgres {

CommandControl Parse(const formats::json::Value& elem,
                     formats::parse::To<CommandControl>);

class Config {
 public:
  taxi_config::Value<CommandControl> default_command_control;
  taxi_config::Value<CommandControlByHandlerMap> handlers_command_control;

  Config(const taxi_config::DocsMap& docs_map);
};

}  // namespace postgres
}  // namespace storages
