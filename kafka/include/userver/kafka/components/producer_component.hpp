#pragma once

#include <string_view>

#include <userver/kafka/producer.hpp>

#include <userver/components/loggable_component_base.hpp>
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
/// Name                     | Description                                      | Default value
/// ------------------------ | ------------------------------------------------ | ---------------
/// delivery_timeout_ms      | time a produced message waits for successful delivery | --
/// queue_buffering_max_ms   | delay to wait for messages to be transmitted to broker | --
/// enable_idempotence       | whether to make producer idempotent | false
/// poll_timeout_ms          | time in milliseconds producer waits for new delivery events | 10
/// send_retries_count       | how many times producer retries transient delivery errors | 5
/// security_protocol        | protocol used to communicate with brokers | --
/// sasl_mechanisms          | SASL mechanism to use for authentication | none
/// ssl_ca_location          | file or directory path to CA certificate(s) for verifying the broker's key | none

// clang-format on

class ProducerComponent final : public components::LoggableComponentBase {
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
