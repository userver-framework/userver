#pragma once

#include <userver/formats/yaml/value.hpp>
#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace yaml_config::impl {

void Validate(const yaml_config::YamlConfig& static_config,
              const formats::yaml::Value& schema);

}  // namespace yaml_config::impl

USERVER_NAMESPACE_END
