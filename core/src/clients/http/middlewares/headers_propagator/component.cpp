#include <userver/clients/http/middlewares/headers_propagator/component.hpp>

#include <clients/http/middlewares/headers_propagator/middleware.hpp>

#include <userver/components/component_config.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::middlewares::headers_propagator {

Component::Component(const components::ComponentConfig& config, const components::ComponentContext& context)
    : ComponentBase(config, context, MiddlewareIndex{1}),
      middleware_(std::make_unique<headers_propagator::Middleware>())
{}

Component::~Component() = default;

http::MiddlewareBase& Component::GetMiddleware() { return *middleware_; }

}  // namespace clients::http::middlewares::headers_propagator

USERVER_NAMESPACE_END
