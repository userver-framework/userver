#pragma once

/// @file userver/components/component_fwd.hpp
/// @brief Forward declarations for components::ComponentContext and
/// components::ComponentConfig; function components::GetCurrentComponentName() and components::GetFsTaskProcessor().

#include <string_view>

#include <userver/engine/task/task_processor_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

class ComponentConfig;

class ComponentContext;

/// @brief Equivalent to @ref components::ComponentContext::GetComponentName, but works with forward declaration of
/// the @ref components::ComponentContext.
std::string_view GetCurrentComponentName(const ComponentContext& context);

/// @brief Returns the `config["fs-task-processor"]` if it is set; otherwise returns the default blocking
/// task processor that was set in components::ManagerControllerComponent.
engine::TaskProcessor& GetFsTaskProcessor(const ComponentConfig& config, const ComponentContext& context);

}  // namespace components

USERVER_NAMESPACE_END
