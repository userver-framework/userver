#include <userver/kafka/components/consumer_component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <kafka/impl/broker_secrets.hpp>
#include <kafka/impl/configuration.hpp>
#include <kafka/impl/consumer.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

ConsumerComponent::ConsumerComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : components::ComponentBase(config, context),
      consumer_(config.Name(), config["topics"].As<std::vector<std::string>>(),
                context.GetTaskProcessor("consumer-task-processor"),
                context.GetTaskProcessor("main-task-processor"),
                config.As<impl::ConsumerConfiguration>(),
                context.FindComponent<components::Secdist>()
                    .Get()
                    .Get<impl::BrokerSecrets>()
                    .GetSecretByComponentName(config.Name()),
                [&config] {
                  impl::ConsumerExecutionParams params{};
                  params.max_batch_size = config["max_batch_size"].As<size_t>();
                  params.poll_timeout =
                      config["poll_timeout"].As<std::chrono::milliseconds>(
                          params.poll_timeout);

                  return params;
                }()) {
  auto& storage =
      context.FindComponent<components::StatisticsStorage>().GetStorage();

  statistics_holder_ = storage.RegisterWriter(
      config.Name(), [this](utils::statistics::Writer& writer) {
        consumer_->DumpMetric(writer);
      });
}

ConsumerComponent::~ConsumerComponent() { statistics_holder_.Unregister(); }

ConsumerScope ConsumerComponent::GetConsumer() {
  return consumer_->MakeConsumerScope();
}

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
    env_pod_name:
        type: string
        description: |
            if defined and `group_id` value contains
            `{pod_name}` substring, the substring
            is replaced with the value of the environment
            variable `env_pod_name`
        defaultDescription: none
    topics:
        type: array
        description: list of topics consumer subscribes
        items:
            type: string
            description: topic name
    enable_auto_commit:
        type: boolean
        description: |
            whether to automatically and periodically commit offsets.
            Note: manual automatic and manual commits is mutual exclsive
        defaultDescription: false
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
