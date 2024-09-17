#pragma once

#include <string_view>

#include <userver/kafka/producer.hpp>

#include <userver/components/component_base.hpp>
#include <userver/utils/statistics/entry.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

// clang-format off

/// @ingroup userver_components
///
/// @brief Apache Kafka Producer client component.
///
/// ## Static configuration example:
///
/// @snippet kafka/functional_tests/integrational_tests/static_config.yaml  Kafka service sample - producer static config
///
/// ## Secdist format
///
/// A Kafka alias in secdist is described as a JSON object
/// `kafka_settings`, containing credentials of Kafka brokers.
///
/// @snippet kafka/functional_tests/integrational_tests/tests/conftest.py  Kafka service sample - secdist
///
/// ## Static options:
/// Name                         | Description                                      | Default value
/// ---------------------------- | ------------------------------------------------ | ---------------
/// client_id                    | An id string to pass to the server when making requests | userver
/// delivery_timeout             | time a produced message waits for successful delivery | --
/// queue_buffering_max          | delay to wait for messages to be transmitted to broker | --
/// enable_idempotence           | whether to make producer idempotent | false
/// queue_buffering_max_messages | maximum number of messages waiting for delivery | 100000
/// queue_buffering_max_kbytes   | maximum size of messages waiting for delivery | 1048576
/// message_max_bytes            | maximum size of message | 1000000
/// message_send_max_retries     | maximum number of send request retries until `delivery_timeout` reached | 2147483647
/// retry_backoff                | backoff time before retrying send request, exponentially increases after each retry | 100
/// retry_backoff_max            | backoff upper bound | 1000
/// security_protocol            | protocol used to communicate with brokers | --
/// sasl_mechanisms              | SASL mechanism to use for authentication | none
/// ssl_ca_location              | file or directory path to CA certificate(s) for verifying the broker's key | none
/// rd_kafka_custom_options  | a map of librdkafka library additional options | '{}'

// clang-format on

class ProducerComponent final : public components::ComponentBase {
 public:
  static constexpr std::string_view kName = "kafka-producer";

  ProducerComponent(const components::ComponentConfig& config,
                    const components::ComponentContext& context);
  ~ProducerComponent() override;

  Producer& GetProducer();

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  Producer producer_;

  /// @note Subscriptions must be the last fields! Add new fields above this
  /// comment.
  utils::statistics::Entry statistics_holder_;
};

}  // namespace kafka

USERVER_NAMESPACE_END
