#include <userver/ugrpc/server/middlewares/deadline_propagation/component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <ugrpc/server/middlewares/deadline_propagation/middleware.hpp>
#include <userver/ugrpc/server/middlewares/congestion_control/component.hpp>
#include <userver/ugrpc/server/middlewares/groups.hpp>
#include <userver/ugrpc/server/middlewares/log/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::deadline_propagation {

Component::Component(const components::ComponentConfig& config, const components::ComponentContext& context)
    : MiddlewareComponentBase(
          config,
          context,
          MiddlewareDependencyBuilder().InGroup<groups::Core>().After<congestion_control::Component>(
              DependencyType::kWeak
          )
      ) {}

std::shared_ptr<MiddlewareBase> Component::GetMiddleware() { return std::make_shared<Middleware>(); }

}  // namespace ugrpc::server::middlewares::deadline_propagation

USERVER_NAMESPACE_END
