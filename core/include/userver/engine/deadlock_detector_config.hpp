#pragma once

#include <userver/formats/yaml.hpp>
#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

enum class DeadlockDetector {
    kOff,
    kOn,
    kDetectOnly,
};

DeadlockDetector Parse(const yaml_config::YamlConfig& value, formats::parse::To<DeadlockDetector>);

}  // namespace engine

USERVER_NAMESPACE_END
