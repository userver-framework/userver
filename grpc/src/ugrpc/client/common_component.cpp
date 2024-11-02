#include <userver/ugrpc/client/common_component.hpp>

#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/logging/level_serialization.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <ugrpc/impl/grpc_native_logging.hpp>
#include <userver/ugrpc/client/middlewares/base.hpp>
#include <userver/ugrpc/server/server_component.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

namespace {

constexpr std::size_t kDefaultCompletionQueueCount = 1;

ugrpc::impl::CompletionQueuePoolBase& FindOrEmplaceCompletionQueues(
    std::optional<impl::CompletionQueuePool>& holder,
    std::size_t queue_count,
    const components::ComponentContext& context
) {
    if (auto* const server = context.FindComponentOptional<server::ServerComponent>()) {
        UINVARIANT(
            queue_count == kDefaultCompletionQueueCount,
            "grpc-client-common.completion-queue-count option is "
            "meaningless and should not be specified if the service has a "
            "grpc-server. Use grpc-server.completion-queue-count instead"
        );
        return server->GetServer().GetCompletionQueues(utils::impl::InternalTag{});
    }
    holder.emplace(queue_count);
    return *holder;
}

}  // namespace

CommonComponent::CommonComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
    : ComponentBase(config, context),
      blocking_task_processor_(context.GetTaskProcessor(config["blocking-task-processor"].As<std::string>())),
      completion_queues_(FindOrEmplaceCompletionQueues(
          client_completion_queues_,
          config["completion-queue-count"].As<std::size_t>(kDefaultCompletionQueueCount),
          context
      )),
      client_statistics_storage_(
          context.FindComponent<components::StatisticsStorage>().GetStorage(),
          ugrpc::impl::StatisticsDomain::kClient
      ) {
    ugrpc::impl::SetupNativeLogging();
    ugrpc::impl::UpdateNativeLogLevel(config["native-log-level"].As<logging::Level>(logging::Level::kError));
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
    completion-queue-count:
        type: integer
        description: |
            completion queue count to create. Should be ~2 times less than worker
            threads for best RPS.
        minimum: 1
)");
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
