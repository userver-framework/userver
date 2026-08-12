#pragma once

/// @file userver/engine/task_processors_load_monitor.hpp
/// @brief @copybrief engine::TaskProcessorsLoadMonitor

#include <memory>

#include <userver/components/component_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// @ingroup userver_components
///
/// @brief Component to monitor CPU usage for every TaskProcessor present in
/// the service, and dump per-thread stats into metrics.
///
/// ## Static options of engine::TaskProcessorsLoadMonitor :
/// @include{doc} scripts/docs/en/components_schema/core/src/engine/task_processors_load_monitor.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
class TaskProcessorsLoadMonitor final : public components::ComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of @ref engine::TaskProcessorsLoadMonitor
    static constexpr std::string_view kName{"task-processors-load-monitor"};

    TaskProcessorsLoadMonitor(const components::ComponentConfig& config, const components::ComponentContext& context);

    ~TaskProcessorsLoadMonitor() override;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace engine

template <>
inline constexpr bool components::kHasValidate<engine::TaskProcessorsLoadMonitor> = true;

template <>
inline constexpr auto components::kConfigFileMode<engine::TaskProcessorsLoadMonitor> = ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
