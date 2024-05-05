#include <userver/kafka/components/consumer_component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <cppkafka/configuration.h>

#include <kafka/configuration.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

ConsumerComponent::ConsumerComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : components::LoggableComponentBase(config, context),
      consumer_(
          MakeConsumerConfiguration(context.FindComponent<components::Secdist>()
                                        .Get()
                                        .Get<BrokerSecrets>(),
                                    config),
          config.Name(), config["topics"].As<std::vector<std::string>>(),
          config["max_batch_size"].As<size_t>(),
          context.GetTaskProcessor("consumer-task-processor"),
          context.GetTaskProcessor("main-task-processor"),
          context.FindComponent<components::StatisticsStorage>().GetStorage()) {
}

ConsumerComponent::~ConsumerComponent() = default;

Consumer& ConsumerComponent::GetConsumer() { return consumer_; }

yaml_config::Schema ConsumerComponent::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<components::LoggableComponentBase>(R"(
type: object
description: Kafka consumer component
additionalProperties: false
properties:
    env_pod_name:
        type: string
        description: TODO
    group_id:
        type: string
        description: consumer group id
    enable_auto_commit:
        type: boolean
        description: TODO
        description: |
            whether to automatically and periodically commit
            offsets in the background
    auto_offset_reset:
        type: string
        description: |
            action to take when there is no
            initial offset in offset store
            or the desired offset is out of range:
            `smallest`, `earliest`, `beginning` - automatically reset
            the offset to the smallest offset
            `largest`, `latest`, `end` - automatically reset
            the offset to the largest offset,
            `error` - trigger an error (ERR__AUTO_OFFSET_RESET)
        enum:
          - smallest
          - earliest
          - beginning
          - largest
          - latest
          - end
          - error
    max_batch_size:
        type: integer
        description: maximum batch size for one callback call
    security_protocol:
        type: string
        description: protocol used to communicate with brokers
        enum:
          - PLAINTEXT
          - SASL_SSL
    sasl_mechanisms:
        type: string
        description: |
            SASL mechanism to use for authentication.
            Must be set if `security_protocol` equals `SASL_SSL`
        defaultDescription: none
        enum:
          - PLAIN
          - SCRAM-SHA-512
    ssl_ca_location:
        type: string
        description: |
            File or directory path to CA certificate(s) for verifying the broker's key.
            Must be set if `security_protocol` equals `SASL_SSL`.
            If set to `probe`, CA certificates are probed from the default certificates paths
        defaultDescription: none
    topics:
        type: array
        description: list of topics consumer will subscribe
        items:
            type: string
            description: topic name
)");
}

}  // namespace kafka

USERVER_NAMESPACE_END
