#include <userver/kafka/producer_component.hpp>

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
      producer_(config.Name(),
                context.GetTaskProcessor("producer-task-processor"),
                config.As<impl::ProducerConfiguration>(),
                context.FindComponent<components::Secdist>()
                    .Get()
                    .Get<impl::BrokerSecrets>()
                    .GetSecretByComponentName(config.Name())) {
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
    client_id:
        type: string
        description: |
            Client identifier.
            May be an arbitrary string.
            Optional, but you should set this property on each instance because it enables you to more easily correlate requests on the broker with the client instance which made it, which can be helpful in debugging and troubleshooting scenarios.
        defaultDescription: userver
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
    queue_buffering_max_messages:
        type: integer
        description: |
             maximum number of messages allowed on the producer queue. In other
             words, maximum number of simultaneously send requests waiting for delivery
        minimum: 0
        maximum: 2147483647
        defaultDescription: 100000
    queue_buffering_max_kbytes:
        type: integer
        description: |
            maximum total message size sum allowed on the producer queue. Has
            higher priority than `queue_buffering_max_messages`, i.e. if this limit is
            exceeded no more send requests are accepted regardless to the number of messages
        minimum: 1
        maximum: 2147483647
        defaultDescription: 1048576
    message_max_bytes:
        type: integer
        description: one message maximum size
        minimum: 1000
        maximum: 1000000000
        defaultDescription: 1000000
    message_send_max_retries:
        type: integer
        description: maximum number of send request retries until `delivery_timeout` reached
        defaultDescription: 2147483647
    retry_backoff:
        type: string
        description: |
            the backoff time before retrying a producer send request.
            It will be backed off exponentially until number of retries is exhausted
            and bounded by `retry_backoff_max`.
            The backoff must fit in [1ms; 300000ms]
        defaultDescription: 100ms
    retry_backoff_max:
        type: string
        description: |
            backoff upper bound.
            The backoff must fit in [1ms; 300000ms]
        defaultDescription: 1000ms
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
    rd_kafka_custom_options:
        type: object
        description: |
            a map of arbitrary `librdkafka` library configuration options.
            Full list of options is available by link: https://github.com/confluentinc/librdkafka/blob/master/CONFIGURATION.md.
            Note: This options is not guaranteed to be supported in userver-kafka, use it at your own risk
        properties: {}
        additionalProperties:
            type: string
            description: librdkafka option value
        defaultDescription: '{}'
)");
}

}  // namespace kafka

USERVER_NAMESPACE_END
