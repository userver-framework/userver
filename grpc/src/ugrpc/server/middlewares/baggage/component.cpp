#include <userver/ugrpc/server/middlewares/baggage/component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <ugrpc/server/middlewares/baggage/middleware.hpp>
#include <userver/ugrpc/server/middlewares/congestion_control/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::baggage {

Component::Component(const components::ComponentConfig& config, const components::ComponentContext& context)
    : MiddlewareComponentBase(config, context) {}

std::shared_ptr<MiddlewareBase> Component::GetMiddleware() { return std::make_shared<Middleware>(); }

}  // namespace ugrpc::server::middlewares::baggage

USERVER_NAMESPACE_END
