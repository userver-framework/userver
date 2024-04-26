#include <userver/kafka/components/producer_component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <cppkafka/configuration.h>

#include <kafka/configuration.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

ProducerComponent::ProducerComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : components::LoggableComponentBase(config, context),
      producer_(
          MakeProducerConfiguration(context.FindComponent<components::Secdist>()
                                        .Get()
                                        .Get<BrokerSecrets>(),
                                    config),
          config.Name(), context.GetTaskProcessor("producer-task-processor"),
          config["is_testsuite_mode"].As<bool>(),
          context.FindComponent<components::StatisticsStorage>().GetStorage()) {
}

ProducerComponent::~ProducerComponent() = default;

Producer& ProducerComponent::GetProducer() { return producer_; }

yaml_config::Schema ProducerComponent::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<components::LoggableComponentBase>(R"(
type: object
description: Kafka producer component
additionalProperties: false
properties:
    security_protocol:
        type: string
        description: TODO
    delivery_timeout_ms:
        type: integer
        description: TODO
    queue_buffering_max_ms:
        type: integer
        description: TODO
    enable_idempotence:
        type: boolean
        description: TODO
    is_testsuite_mode:
        type: boolean
        description: TODO
)");
}

}  // namespace kafka

USERVER_NAMESPACE_END
