#pragma once

/// @file userver/components/process_starter.hpp
/// @brief @copybrief components::ProcessStarter

#include <userver/components/component_base.hpp>

#include <userver/engine/subprocess/process_starter.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

// clang-format off

/// @ingroup userver_components
///
/// @brief Component for getting the engine::subprocess::ProcessStarter.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// task_processor | the name of the TaskProcessor for asynchronous process starting | `main-task-processor`

// clang-format on
class ProcessStarter final : public ComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of components::ProcessStarter component
    static constexpr std::string_view kName = "process-starter";

    ProcessStarter(const ComponentConfig& config, const ComponentContext& context);

    engine::subprocess::ProcessStarter& Get() { return process_starter_; }

    static yaml_config::Schema GetStaticConfigSchema();

private:
    engine::subprocess::ProcessStarter process_starter_;
};

template <>
inline constexpr bool kHasValidate<ProcessStarter> = true;

}  // namespace components

USERVER_NAMESPACE_END
