#include <userver/ugrpc/server/middlewares/graceful_shutdown_headers/component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/dynamic_config/storage/component.hpp>

#include <ugrpc/server/middlewares/graceful_shutdown_headers/middleware.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::graceful_shutdown_headers {

Component::Component(const components::ComponentConfig& config, const components::ComponentContext& context)
    : MiddlewareFactoryComponentBase(
          config,
          context,
          USERVER_NAMESPACE::middlewares::MiddlewareDependencyBuilder()
              .InGroup<USERVER_NAMESPACE::middlewares::groups::PreCore>()
      ),
      state_(context),
      source_(context.FindComponent<components::DynamicConfig>().GetSource())
{}

std::shared_ptr<const MiddlewareBase>
Component::CreateMiddleware(const ugrpc::server::ServiceInfo&, const yaml_config::YamlConfig&) const {
    return std::make_shared<Middleware>(state_, source_);
}

}  // namespace ugrpc::server::middlewares::graceful_shutdown_headers

USERVER_NAMESPACE_END
