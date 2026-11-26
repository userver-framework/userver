#include <userver/ugrpc/tests/standalone_client.hpp>

#include <utility>

#include <fmt/format.h>

#include <userver/engine/task/task_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::tests {

StandaloneClientFactory::StandaloneClientFactory(client::ClientFactorySettings&& client_factory_settings)
    : metrics_storage_(utils::statistics::MakeBoundMetricsStorage(statistics_storage_, std::nullopt)),
      client_factory_{
          std::move(client_factory_settings),
          engine::current_task::GetTaskProcessor(),
          simple_client_middleware_pipeline_,
          completion_queues_,
          *client_statistics_storage_,
          *metrics_storage_,
          testsuite_control_,
          config_storage_.GetSource(),
      }
{}

std::string MakeIpv6Endpoint(std::uint16_t port) { return fmt::format("[::1]:{}", port); }

}  // namespace ugrpc::tests

USERVER_NAMESPACE_END
