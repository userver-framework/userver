#include <userver/ugrpc/client/middlewares/testsuite/component.hpp>

#include <userver/ugrpc/client/middlewares/testsuite/middleware.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::testsuite {

Component::Component(const components::ComponentConfig& config, const components::ComponentContext& context)
    : MiddlewareFactoryComponentBase(
          config,
          context,
          USERVER_NAMESPACE::middlewares::MiddlewareDependencyBuilder()
              .InGroup<USERVER_NAMESPACE::middlewares::groups::PostCore>()
      ) {}

std::shared_ptr<const MiddlewareBase>
Component::CreateMiddleware(const ClientInfo& info, const yaml_config::YamlConfig& /*middleware_config*/) const {
    return std::make_shared<Middleware>(info.client_name);
}

}  // namespace ugrpc::client::middlewares::testsuite

USERVER_NAMESPACE_END
