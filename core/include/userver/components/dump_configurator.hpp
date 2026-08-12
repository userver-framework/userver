#pragma once

/// @file userver/components/dump_configurator.hpp
/// @brief @copybrief components::DumpConfigurator

#include <string>

#include <userver/components/component_base.hpp>
#include <userver/components/component_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

/// @ingroup userver_components
///
/// @brief Helper component that manages common configuration for userver dumps.
///
/// The component must be configured in service config.
///
/// ## Static options of components::DumpConfigurator :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/dump_configurator.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
///
/// ## Config example:
///
/// @snippet components/common_component_list_test.cpp Sample dump configurator component config
class DumpConfigurator final : public ComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of @ref components::DumpConfigurator component
    static constexpr std::string_view kName = "dump-configurator";

    DumpConfigurator(const ComponentConfig& config, const ComponentContext& context);

    const std::string& GetDumpRoot() const;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    const std::string dump_root_;
};

template <>
inline constexpr bool kHasValidate<DumpConfigurator> = true;

}  // namespace components

USERVER_NAMESPACE_END
