#pragma once

/// @file
/// @brief @copybrief components::ComponentConfig

#include <cstdint>
#include <string>
#include <unordered_map>

#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

/// @brief Represents the config of a component that is being constructed;
/// see @ref scripts/docs/en/userver/component_system.md for introduction into components.
class ComponentConfig final : public yaml_config::YamlConfig {
public:
    /// Creates an empty config
    explicit ComponentConfig(std::string name);

    ComponentConfig(yaml_config::YamlConfig value);

    /// Name of the current component
    const std::string& Name() const;

    void SetName(std::string name);

private:
    std::string name_;
};

ComponentConfig Parse(const yaml_config::YamlConfig& value, formats::parse::To<ComponentConfig>);

using ComponentConfigMap = std::unordered_map<std::string, const ComponentConfig&>;

}  // namespace components

USERVER_NAMESPACE_END
