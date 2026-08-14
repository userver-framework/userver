#include <userver/ugrpc/client/common_component.hpp>

#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/logging/level_serialization.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <ugrpc/impl/grpc_native_logging.hpp>
#include <userver/ugrpc/server/server_component.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/ugrpc/client/common_component.yaml.hpp"  // Y_IGNORE
#endif

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

std::optional<std::string> ParseRetryLimiterFactoryName(const components::ComponentConfig& config) {
    auto retry_limiter_enabled = config["retry-limiter-enabled"].As<bool>(false);
    if (!retry_limiter_enabled) {
        return std::nullopt;
    }

    // Deliberately no component lookup here: see the doc comment on
    // CommonComponent::retry_limiter_factory_name_ for why.
    return config["retry-limiter"].As<std::optional<std::string>>();
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
          context.Scopes(),
          context.FindComponent<components::StatisticsStorage>().GetStorage(),
          ugrpc::impl::StatisticsDomain::kClient
      ),
      proxy_settings_{
          config["proxy-address"].As<std::string>(""),
          config["servicemesh-settings"]["egress"]["disable_proxy"].As<std::unordered_set<std::string>>({})
      },
      retry_limiter_factory_name_{ParseRetryLimiterFactoryName(config)}
{
    ugrpc::impl::SetupNativeLogging();
    ugrpc::impl::UpdateNativeLogLevel(config["native-log-level"].As<logging::Level>(logging::Level::kError));
}

CommonComponent::~CommonComponent() = default;

yaml_config::Schema CommonComponent::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<components::ComponentBase>("src/ugrpc/client/common_component.yaml");
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
