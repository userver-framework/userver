#pragma once

/// @file userver/dynamic_config/client/component.hpp
/// @brief @copybrief components::DynamicConfigClient

#include <userver/components/component_base.hpp>
#include <userver/dynamic_config/client/client.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

/// @ingroup userver_components
///
/// @brief Component that starts a clients::dynamic_config::Client client.
///
/// Returned references to clients::dynamic_config::Client live for a lifetime
/// of the component and are safe for concurrent use.
///
/// The component must be configured in service config.
///
/// ## Static options of components::DynamicConfigClient :
/// @include{doc} scripts/docs/en/components_schema/core/src/dynamic_config/client/component.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
///
/// ## Static configuration example:
///
/// @snippet components/common_component_list_test.cpp  Sample dynamic configs client component config
class DynamicConfigClient : public ComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of @ref components::DynamicConfigClient
    static constexpr std::string_view kName = "dynamic-config-client";

    DynamicConfigClient(const ComponentConfig&, const ComponentContext&);
    ~DynamicConfigClient() override = default;

    [[nodiscard]] dynamic_config::Client& GetClient() const;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    std::unique_ptr<dynamic_config::Client> config_client_;
};

template <>
inline constexpr bool kHasValidate<DynamicConfigClient> = true;

}  // namespace components

USERVER_NAMESPACE_END
