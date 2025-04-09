#pragma once

/// @file userver/components/component_fwd.hpp
/// @brief Forward declarations for components::ComponentContext and
/// components::ComponentConfig; function components::GetCurrentComponentName().

#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace components {

class ComponentConfig;

class ComponentContext;

/// @brief Equivalent to @ref components::ComponentContext::GetComponentName, but works with forward declaration of
/// the @ref components::ComponentContext.
std::string_view GetCurrentComponentName(const ComponentContext& context);

}  // namespace components

USERVER_NAMESPACE_END
