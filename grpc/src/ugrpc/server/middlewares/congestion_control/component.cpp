#include <userver/ugrpc/server/middlewares/congestion_control/component.hpp>

#include <ugrpc/server/middlewares/congestion_control/middleware.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/congestion_control/component.hpp>
#include <userver/ugrpc/server/server_component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::congestion_control {

Component::Component(const components::ComponentConfig& config,
                     const components::ComponentContext& context)
    : MiddlewareComponentBase(config, context),
      middleware_(std::make_shared<Middleware>()) {
  auto& cc_component =
      context.FindComponent<USERVER_NAMESPACE::congestion_control::Component>();

  auto& server_limiter = cc_component.GetServerLimiter();
  server_limiter.RegisterLimitee(*middleware_);

  auto& server = context.FindComponent<ServerComponent>().GetServer();
  auto& server_sensor = cc_component.GetServerSensor();
  server_sensor.RegisterRequestsSource(server);
}

std::shared_ptr<MiddlewareBase> Component::GetMiddleware() {
  return middleware_;
}

yaml_config::Schema Component::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<MiddlewareComponentBase>(R"(
type: object
description: gRPC service congestion control middleware component
additionalProperties: false
properties: {}
)");
}

}  // namespace ugrpc::server::middlewares::congestion_control

USERVER_NAMESPACE_END
