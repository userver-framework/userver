#pragma once

/// @file userver/clients/http/midddlewares/retry_budget/component.hpp
/// @brief @copybrief clients::http::middlewares::retry_budget::Component

#include <memory>

#include <userver/clients/http/middlewares/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::middlewares::retry_budget {

class Middleware;

class Component final : public ComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of
    /// clients::http::middlewares::retry_budget::Component component
    static constexpr std::string_view kName = "http-client-retry-budget";

    Component(const components::ComponentConfig&, const components::ComponentContext&);

    ~Component() override;

    http::MiddlewareBase& GetMiddleware() override;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    std::unique_ptr<Middleware> middleware_;
};

}  // namespace clients::http::middlewares::retry_budget

template <>
inline constexpr bool components::kHasValidate<clients::http::middlewares::retry_budget::Component> = true;

USERVER_NAMESPACE_END
