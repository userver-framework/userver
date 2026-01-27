#pragma once

/// @file userver/components/process_starter.hpp
/// @brief @copybrief components::ProcessStarter

#include <userver/components/component_base.hpp>

#include <userver/engine/subprocess/process_starter.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

/// @ingroup userver_components
///
/// @brief Component for getting the engine::subprocess::ProcessStarter.
///
/// ## Static options of components::ProcessStarter :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/process_starter.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
class ProcessStarter final : public ComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of @ref components::ProcessStarter component
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
