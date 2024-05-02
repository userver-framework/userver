#include <userver/kafka/components/producer_component.hpp>

#include <userver/components/statistics_storage.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <kafka/impl/configuration.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

ProducerComponent::ProducerComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : components::LoggableComponentBase(config, context),
      producer_(
          std::make_unique<Configuration>(config, context), config.Name(),
          context.GetTaskProcessor("producer-task-processor"),
          config["poll_timeout_ms"].As<std::chrono::milliseconds>(
              Producer::kDefaultPollTimeout),
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
    delivery_timeout_ms:
        type: integer
        description: time a produced message waits for successful delivery
    queue_buffering_max_ms:
        type: integer
        description: delay to wait for messages to be transmitted to broker
    enable_idempotence:
        type: boolean
        description: whether to make producer idempotent
        defaultDescription: false
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
    is_testsuite_mode:
        type: boolean
        description: whether to use Kafka Server mocks instead of real Kafka Broker
)");
}

}  // namespace kafka

USERVER_NAMESPACE_END
