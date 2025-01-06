#pragma once

#include <memory>

#include <userver/clients/http/plugin_component.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::plugins::headers_propagator {

class Plugin;

class Component final : public plugin::ComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of
    /// clients::http::plugins::headers_propagator::Component component
    static constexpr std::string_view kName = "http-client-plugin-headers-propagator";

    Component(const components::ComponentConfig&, const components::ComponentContext&);

    ~Component() override;

    http::Plugin& GetPlugin() override;

private:
    std::unique_ptr<headers_propagator::Plugin> plugin_;
};

}  // namespace clients::http::plugins::headers_propagator

template <>
inline constexpr bool components::kHasValidate<clients::http::plugins::headers_propagator::Component> = true;

USERVER_NAMESPACE_END
