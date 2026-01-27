#pragma once

/// @file userver/clients/dns/component.hpp
/// @brief @copybrief clients::dns::Component

#include <userver/clients/dns/resolver.hpp>
#include <userver/components/component_base.hpp>
#include <userver/utils/statistics/entry.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::dns {

/// @ingroup userver_components
///
/// @brief Caching DNS resolver component.
///
/// Returned references to clients::dns::Resolver live for a lifetime
/// of the component and are safe for concurrent use.
///
/// ## Static options of clients::dns::Component :
/// @include{doc} scripts/docs/en/components_schema/core/static_configs/dns_client.md
///
/// ## Static configuration example:
///
/// @snippet components/common_component_list_test.cpp  Sample dns client component config
class Component final : public components::ComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of clients::dns::Component component
    static constexpr std::string_view kName = "dns-client";

    Component(const components::ComponentConfig&, const components::ComponentContext&);

    Resolver& GetResolver();

private:
    void Write(utils::statistics::Writer& writer);

    Resolver resolver_;
};

}  // namespace clients::dns

template <>
inline constexpr bool components::kForceNoValidation<clients::dns::Component> = true;

template <>
inline constexpr auto components::kConfigFileMode<clients::dns::Component> = ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
