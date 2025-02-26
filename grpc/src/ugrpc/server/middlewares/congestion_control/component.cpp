#include <userver/ugrpc/server/middlewares/congestion_control/component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/congestion_control/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <ugrpc/server/middlewares/congestion_control/middleware.hpp>
#include <userver/ugrpc/server/middlewares/groups.hpp>
#include <userver/ugrpc/server/middlewares/log/component.hpp>
#include <userver/ugrpc/server/server_component.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::congestion_control {

Component::Component(const components::ComponentConfig& config, const components::ComponentContext& context)
    : MiddlewareFactoryComponentBase(
          config,
          context,
          ugrpc::middlewares::MiddlewareDependencyBuilder().InGroup<groups::Core>()
      ),
      middleware_(std::make_shared<Middleware>()) {
    auto& cc_component = context.FindComponent<USERVER_NAMESPACE::congestion_control::Component>();

    auto& server_limiter = cc_component.GetServerLimiter();
    server_limiter.RegisterLimitee(*middleware_);

    auto& server = context.FindComponent<ServerComponent>().GetServer();
    auto& server_sensor = cc_component.GetServerSensor();
    server_sensor.RegisterRequestsSource(server);
}

std::shared_ptr<MiddlewareBase> Component::CreateMiddleware(const ServiceInfo&, const yaml_config::YamlConfig&) const {
    return middleware_;
}

}  // namespace ugrpc::server::middlewares::congestion_control

USERVER_NAMESPACE_END
