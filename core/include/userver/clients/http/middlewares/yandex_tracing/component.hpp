#pragma once

/// @file userver/clients/http/midddlewares/yandex_tracing/component.hpp
/// @brief @copybrief clients::http::middlewares::yandex_tracing::Component

#include <memory>

#include <userver/clients/http/middlewares/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::middlewares::yandex_tracing {

class Middleware;

class Component final : public ComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of
    /// clients::http::middlewares::yandex_tracing::Component component
    static constexpr std::string_view kName = "http-client-yandex-tracing";

    Component(const components::ComponentConfig&, const components::ComponentContext&);

    ~Component() override;

    http::MiddlewareBase& GetMiddleware() override;

private:
    std::unique_ptr<Middleware> middleware_;
};

}  // namespace clients::http::middlewares::yandex_tracing

template <>
inline constexpr bool components::kHasValidate<clients::http::middlewares::yandex_tracing::Component> = true;

USERVER_NAMESPACE_END
