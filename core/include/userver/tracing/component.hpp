#pragma once

/// @file userver/tracing/component.hpp
/// @brief @copybrief components::Tracer

#include <userver/components/component_fwd.hpp>
#include <userver/components/raw_component_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

/// @ingroup userver_components
///
/// @brief Component that initializes the request tracing facilities.
///
/// Finds the components::Logging component.
///
/// The component must be configured in service config.
///
/// ## Static options of components::Tracer :
/// @include{doc} scripts/docs/en/components_schema/core/src/tracing/component.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
///
/// ## Static configuration example:
///
/// @snippet components/common_component_list_test.cpp  Sample tracer component config
class Tracer final : public RawComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of @ref components::Tracer
    static constexpr std::string_view kName = "tracer";

    Tracer(const ComponentConfig& config, const ComponentContext& context);

    static yaml_config::Schema GetStaticConfigSchema();
};

template <>
inline constexpr bool kHasValidate<Tracer> = true;

template <>
inline constexpr auto kConfigFileMode<Tracer> = ConfigFileMode::kNotRequired;

}  // namespace components

USERVER_NAMESPACE_END
