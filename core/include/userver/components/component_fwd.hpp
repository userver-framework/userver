#pragma once

/// @file userver/components/component_fwd.hpp
/// @brief Forward declarations for components::ComponentContext and
/// components::ComponentConfig; function components::GetCurrentComponentName()

#include <string>

USERVER_NAMESPACE_BEGIN

namespace components {

class ComponentConfig;

class ComponentContext;

/// @brief Equivalent to config.Name() but works with forward declaration of
/// the components::ComponentConfig.
std::string GetCurrentComponentName(const ComponentConfig& config);

}  // namespace components

USERVER_NAMESPACE_END
