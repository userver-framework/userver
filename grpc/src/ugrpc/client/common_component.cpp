#include <userver/ugrpc/client/common_component.hpp>

#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/logging/level_serialization.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <ugrpc/impl/logging.hpp>
#include <userver/ugrpc/client/middlewares/base.hpp>
#include <userver/ugrpc/server/server_component.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

namespace {

grpc::CompletionQueue& FindOrEmplaceQueue(
    std::optional<QueueHolder>& holder,
    const components::ComponentContext& context) {
  if (auto* const server =
          context.FindComponentOptional<server::ServerComponent>()) {
    return server->GetServer().GetCompletionQueue();
  }
  holder.emplace();
  return holder->GetQueue();
}

}  // namespace

CommonComponent::CommonComponent(const components::ComponentConfig& config,
                                 const components::ComponentContext& context)
    : ComponentBase(config, context),
      blocking_task_processor_(context.GetTaskProcessor(
          config["blocking-task-processor"].As<std::string>())),
      queue_(FindOrEmplaceQueue(queue_holder_, context)),
      client_statistics_storage_(
          context.FindComponent<components::StatisticsStorage>().GetStorage(),
          ugrpc::impl::StatisticsDomain::kClient) {
  ugrpc::impl::SetupNativeLogging();
  ugrpc::impl::UpdateNativeLogLevel(
      config["native-log-level"].As<logging::Level>(logging::Level::kError));
}

CommonComponent::~CommonComponent() = default;

yaml_config::Schema CommonComponent::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<components::ComponentBase>(R"(
type: object
description: Provides a ClientFactory in the component system
additionalProperties: false
properties:
    blocking-task-processor:
        type: string
        description: the task processor for blocking channel creation
    native-log-level:
        type: string
        description: min log level for the native gRPC library
        defaultDescription: error
        enum:
          - debug
          - info
          - error
)");
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
