#include <userver/logging/level_serialization.hpp>

#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

Level Parse(const yaml_config::YamlConfig& value, formats::parse::To<Level>) {
  return logging::LevelFromString(value.As<std::string>());
}

}  // namespace logging

USERVER_NAMESPACE_END
