#include <userver/clients/http/middlewares/yandex_tracing/component.hpp>

#include <clients/http/middlewares/yandex_tracing/middleware.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::middlewares::yandex_tracing {

Component::Component(const components::ComponentConfig& config, const components::ComponentContext& context)
    : ComponentBase(config, context, MiddlewareIndex{0}),
      middleware_(std::make_unique<yandex_tracing::Middleware>())
{}

Component::~Component() = default;

http::MiddlewareBase& Component::GetMiddleware() { return *middleware_; }

}  // namespace clients::http::middlewares::yandex_tracing

USERVER_NAMESPACE_END
