#pragma once

/// @file userver/clients/http/plugins/retry_budget/component.hpp
/// @brief @copybrief clients::http::plugins::retry_budget::Component

#include <memory>

#include <userver/clients/http/plugin_component.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::plugins::retry_budget {

class Plugin;

class Component final : public plugin::ComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of
    /// clients::http::plugins::retry_budget::Component component
    static constexpr std::string_view kName = "http-client-plugin-retry-budget";

    Component(const components::ComponentConfig&, const components::ComponentContext&);

    ~Component() override;

    http::Plugin& GetPlugin() override;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    std::unique_ptr<retry_budget::Plugin> plugin_;
};

}  // namespace clients::http::plugins::retry_budget

template <>
inline constexpr bool components::kHasValidate<clients::http::plugins::retry_budget::Component> = true;

USERVER_NAMESPACE_END
