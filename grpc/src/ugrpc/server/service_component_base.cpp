#include <userver/ugrpc/server/service_component_base.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/utils/assert.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/ugrpc/server/middlewares/base.hpp>
#include <userver/ugrpc/server/server_component.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

ServiceComponentBase::ServiceComponentBase(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : LoggableComponentBase(config, context),
      server_(context.FindComponent<ServerComponent>().GetServer()),
      service_task_processor_(context.GetTaskProcessor(
          config["task-processor"].As<std::string>())) {
  auto middleware_names = config["middlewares"].As<std::vector<std::string>>();
  for (const auto& name : middleware_names) {
    auto& component = context.FindComponent<MiddlewareComponentBase>(name);
    middlewares_.push_back(component.GetMiddleware());
  }
}

void ServiceComponentBase::RegisterService(ServiceBase& service) {
  UASSERT_MSG(!registered_.exchange(true), "Register must only be called once");
  server_.AddService(service, service_task_processor_, middlewares_);
}

yaml_config::Schema ServiceComponentBase::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<components::LoggableComponentBase>(R"(
type: object
description: base class for all the gRPC service components
additionalProperties: false
properties:
    task-processor:
        type: string
        description: the task processor to use for responses
    middlewares:
        type: array
        items:
            type: string
            description: middleware component name
        description: middlewares names to use
)");
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
