#include <userver/kafka/components/producer_component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/utils/statistics/writer.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <kafka/impl/broker_secrets.hpp>
#include <kafka/impl/configuration.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

ProducerComponent::ProducerComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : components::ComponentBase(config, context),
      producer_(
          config.Name(), context.GetTaskProcessor("producer-task-processor"),
          config.As<impl::ProducerConfiguration>(),
          context.FindComponent<components::Secdist>()
              .Get()
              .Get<impl::BrokerSecrets>()
              .GetSecretByComponentName(config.Name()),
          [&config] {
            ProducerExecutionParams params{};
            params.poll_timeout =
                config["poll_timeout"].As<std::chrono::milliseconds>(
                    params.poll_timeout);
            params.send_retries = config["send_retries_count"].As<std::size_t>(
                params.send_retries);

            return params;
          }()) {
  auto& storage =
      context.FindComponent<components::StatisticsStorage>().GetStorage();

  statistics_holder_ = storage.RegisterWriter(
      config.Name(), [this](utils::statistics::Writer& writer) {
        producer_.DumpMetric(writer);
      });
}

ProducerComponent::~ProducerComponent() { statistics_holder_.Unregister(); }

Producer& ProducerComponent::GetProducer() { return producer_; }

yaml_config::Schema ProducerComponent::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<components::ComponentBase>(R"(
type: object
description: Kafka producer component
additionalProperties: false
properties:
    delivery_timeout:
        type: string
        description: time a produced message waits for successful delivery
    queue_buffering_max:
        type: string
        description: delay to wait for messages to be transmitted to broker
    enable_idempotence:
        type: boolean
        description: whether to make producer idempotent
        defaultDescription: false
    poll_timeout:
        type: string
        description: time producer waits for new delivery events
        defaultDescription: 10ms
    send_retries_count:
        type: integer
        description: how many times producer retries transient delivery errors
        defaultDescription: 5
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
            file or directory path to CA certificate(s) for verifying the broker's key.
            Must be set if `security_protocol` equals `SASL_SSL`.
            If set to `probe`, CA certificates are probed from the default certificates paths
        defaultDescription: none
)");
}

}  // namespace kafka

USERVER_NAMESPACE_END
