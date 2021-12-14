#include <userver/ugrpc/server/handler_component_base.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/utils/assert.hpp>

#include <userver/ugrpc/server/server_component.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

HandlerComponentBase::HandlerComponentBase(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : LoggableComponentBase(config, context),
      server_(context.FindComponent<ServerComponent>().GetServer()),
      handler_task_processor_(context.GetTaskProcessor(
          config["task-processor"].As<std::string>())) {}

void HandlerComponentBase::CheckRegisterIsCalledOnce() {
  UASSERT_MSG(!registered_.exchange(true), "Register must only be called once");
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
