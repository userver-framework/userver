#include "component.hpp"
#include "middleware.hpp"

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/components/component_config.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace sample::grpc::auth::client {

Component::Component(const components::ComponentConfig& config,
                     const components::ComponentContext& context)
    : MiddlewareComponentBase(config, context),
      factory_(std::make_shared<MiddlewareFactory>(context)) {}

std::shared_ptr<const ugrpc::client::MiddlewareFactoryBase>
Component::GetMiddlewareFactory() {
  return factory_;
}

}  // namespace sample::grpc::auth::client
