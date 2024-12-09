#include <userver/kafka/consumer_component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/kafka/impl/broker_secrets.hpp>
#include <userver/kafka/impl/configuration.hpp>
#include <userver/kafka/impl/consumer.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

ConsumerComponent::ConsumerComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : components::ComponentBase(config, context),
      consumer_(
          config.Name(),
          config["topics"].As<std::vector<std::string>>(),
          context.GetTaskProcessor("consumer-task-processor"),
          context.GetTaskProcessor("consumer-blocking-task-processor"),
          context.GetTaskProcessor("main-task-processor"),
          config.As<impl::ConsumerConfiguration>(),
          context.FindComponent<components::Secdist>().Get().Get<impl::BrokerSecrets>().GetSecretByComponentName(
              config.Name()
          ),
          [&config] {
              impl::ConsumerExecutionParams params{};
              params.max_batch_size = config["max_batch_size"].As<std::size_t>(params.max_batch_size);
              params.poll_timeout = config["poll_timeout"].As<std::chrono::milliseconds>(params.poll_timeout);
              params.max_callback_duration =
                  config["max_callback_duration"].As<std::chrono::milliseconds>(params.max_callback_duration);
              params.restart_after_failure_delay =
                  config["restart_after_failure_delay"].As<std::chrono::milliseconds>(params.restart_after_failure_delay
                  );

              return params;
          }()
      ) {
    auto& storage = context.FindComponent<components::StatisticsStorage>().GetStorage();

    statistics_holder_ = storage.RegisterWriter(config.Name(), [this](utils::statistics::Writer& writer) {
        consumer_->DumpMetric(writer);
    });
}

ConsumerComponent::~ConsumerComponent() { statistics_holder_.Unregister(); }

ConsumerScope ConsumerComponent::GetConsumer() { return consumer_->MakeConsumerScope(); }

yaml_config::Schema ConsumerComponent::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<components::ComponentBase>(R"(
type: object
description: Kafka consumer component
additionalProperties: false
properties:
    group_id:
        type: string
        description: |
            consumer group id.
            Topic partition evenly distributed
            between consumers with the same `group_id`
    client_id:
        type: string
        description: |
            Client identifier.
            May be an arbitrary string.
            Optional, but you should set this property on each instance because it enables you to more easily correlate requests on the broker with the client instance which made it, which can be helpful in debugging and troubleshooting scenarios.
        defaultDescription: userver
    topics:
        type: array
        description: list of topics consumer subscribes
        items:
            type: string
            description: topic name
    max_batch_size:
        type: integer
        description: maximum number of messages consumer waits for new messages before calling a callback
        defaultDescription: 1
    poll_timeout:
        type: string
        description: maximum amount of time consumer waits for new messages before calling a callback
        defaultDescription: 1s
    max_callback_duration:
        type: string
        description: |
            duration user callback must fit not to be kicked from the consumer group.
            The duration must fit in [1ms; 86400000ms]
        defaultDescription: 5m
    restart_after_failure_delay:
        type: string
        description: backoff consumer waits until restart after user-callback exception.
        defaultDescription: 10s
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
            `error` - trigger an error (ERR__AUTO_OFFSET_RESET).
            Note: the policy applies iff there are not committed
            offsets in topic
        enum:
          - smallest
          - earliest
          - beginning
          - largest
          - latest
          - end
          - error
    env_pod_name:
        type: string
        description: |
            if defined and `group_id` value contains
            `{pod_name}` substring, the substring
            is replaced with the value of the environment
            variable `env_pod_name`
        defaultDescription: none
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
    topic_metadata_refresh_interval:
        type: string
        description: |
            period of time at which
            topic and broker metadata is refreshed
            in order to discover any new brokers,
            topics, partitions or partition leader changes
        defaultDescription: 5m
    metadata_max_age:
        type: string
        description: |
            metadata cache max age.
            Recommended value is 3 times `topic_metadata_refresh_interval`
        defaultDescription: 15m
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
