#pragma once

/// @file userver/engine/deadlock_detector_config.hpp
/// @brief Engine deadlock detector mode and parsing

#include <userver/formats/yaml.hpp>
#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// @brief Deadlock detector operating mode
enum class DeadlockDetector {
    kOff,
    kOn,
    kDetectOnly,
};

DeadlockDetector Parse(const yaml_config::YamlConfig& value, formats::parse::To<DeadlockDetector>);

}  // namespace engine

USERVER_NAMESPACE_END
