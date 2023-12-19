#pragma once

/// @file userver/components/process_starter.hpp
/// @brief @copybrief components::ProcessStarter

#include <userver/components/loggable_component_base.hpp>

#include <userver/engine/subprocess/process_starter.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

/// @ingroup userver_components
///
/// @brief Component for getting the engine::subprocess::ProcessStarter.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// task_processor | the name of the TaskProcessor for process starting | -
class ProcessStarter : public LoggableComponentBase {
 public:
  ProcessStarter(const ComponentConfig& config,
                 const ComponentContext& context);

  /// @ingroup userver_component_names
  /// @brief The default name of components::ProcessStarter component
  static constexpr std::string_view kName = "process-starter";

  engine::subprocess::ProcessStarter& Get() { return process_starter_; }

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  engine::subprocess::ProcessStarter process_starter_;
};

template <>
inline constexpr bool kHasValidate<ProcessStarter> = true;

}  // namespace components

USERVER_NAMESPACE_END
