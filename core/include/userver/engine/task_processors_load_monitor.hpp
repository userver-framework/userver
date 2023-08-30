#pragma once

/// @file userver/engine/task_processors_load_monitor.hpp
/// @brief @copybrief engine::TaskProcessorsLoadMonitor

#include <memory>

#include <userver/components/loggable_component_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

// clang-format off
/// @ingroup userver_components
///
/// @brief Component to monitor CPU usage for every TaskProcessor present in
/// the service, and dump per-thread stats into metrics.
///
/// ## Static options:
/// Inherits all the options from components::LoggableComponentBase and adds the
/// following ones:
///
/// Name           | Description                                    | Default value
/// -------------- | ---------------------------------------------- | ---------------------------------
/// task-processor | name of the TaskProcessor to run monitoring on | default monitoring task processor
// clang-format on
class TaskProcessorsLoadMonitor final
    : public components::LoggableComponentBase {
 public:
  /// @ingroup userver_component_names
  /// @brief The default name of engine::TaskProcessorsLoadMonitor
  static constexpr std::string_view kName{"task-processors-load-monitor"};

  TaskProcessorsLoadMonitor(const components::ComponentConfig& config,
                            const components::ComponentContext& context);

  ~TaskProcessorsLoadMonitor() override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace engine

template <>
inline constexpr bool
    components::kHasValidate<engine::TaskProcessorsLoadMonitor> = true;

template <>
inline constexpr auto
    components::kConfigFileMode<engine::TaskProcessorsLoadMonitor> =
        ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
