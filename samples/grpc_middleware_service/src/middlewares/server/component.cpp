#include "component.hpp"
#include "middleware.hpp"

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/components/component_context.hpp>
/// [gRPC middleware sample - middleware registration]
#include <userver/ugrpc/server/middlewares/groups.hpp>

namespace sample::grpc::auth::server {

Component::Component(const components::ComponentConfig& config, const components::ComponentContext& context)
    : MiddlewareFactoryComponentBase(
          config,
          context,
          ugrpc::middlewares::MiddlewareDependencyBuilder().InGroup<ugrpc::server::groups::Auth>()
      ),
      middleware_(std::make_shared<Middleware>()) {}

std::shared_ptr<ugrpc::server::MiddlewareBase>
Component::CreateMiddleware(const ugrpc::server::ServiceInfo&, const yaml_config::YamlConfig&) const {
    return middleware_;
}

}  // namespace sample::grpc::auth::server
/// [gRPC middleware sample - middleware registration]
