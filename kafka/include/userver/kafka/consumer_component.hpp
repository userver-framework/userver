#pragma once

#include <string_view>

#include <userver/kafka/consumer_scope.hpp>

#include <userver/components/component_base.hpp>
#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/statistics/entry.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

namespace impl {

class Consumer;

}  // namespace impl

// clang-format off

/// @ingroup userver_components
///
/// @brief Apache Kafka Consumer client component.
///
/// ## Static configuration example:
///
/// @snippet kafka/functional_tests/integrational_tests/static_config.yaml  Kafka service sample - consumer static config
///
/// ## Secdist format
///
/// A Kafka alias in secdist is described as a JSON object
/// `kafka_settings`, containing credentials of Kafka brokers.
///
/// @snippet kafka/functional_tests/integrational_tests/tests/conftest.py  Kafka service sample - secdist
///
/// ## Static options:
/// Name                               | Description                                      | Default value
/// ---------------------------------- | ------------------------------------------------ | ---------------
/// group_id                           | consumer group id | --
/// topics                             | list of topics consumer subscribes | --
/// enable_auto_commit                 | whether to automatically and periodically commit offsets | false
/// auto_offset_reset                  | action to take when there is no initial offset in offset store | smallest
/// max_batch_size                     | maximum batch size for one callback call | --
/// env_pod_name                       | environment variable to substitute `{pod_name}` substring in `group_id` | none
/// security_protocol                  | protocol used to communicate with brokers | --
/// sasl_mechanisms                    | SASL mechanism to use for authentication | none
/// ssl_ca_location                    | File or directory path to CA certificate(s) for verifying the broker's key | 5m
/// topic_metadata_refresh_interval    | period of time at which topic and broker metadata is refreshed | 15m
/// metadata_max_age                   | metadata cache max age | none
/// rd_kafka_custom_options            | a map of librdkafka library additional options | '{}'

// clang-format on

class ConsumerComponent final : public components::ComponentBase {
 public:
  static constexpr std::string_view kName = "kafka-consumer";

  ConsumerComponent(const components::ComponentConfig& config,
                    const components::ComponentContext& context);
  ~ConsumerComponent() override;

  ConsumerScope GetConsumer();

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  static constexpr std::size_t kImplSize = 2480;
  static constexpr std::size_t kImplAlign = 16;
  utils::FastPimpl<impl::Consumer, kImplSize, kImplAlign> consumer_;

  /// @note Subscriptions must be the last fields! Add new fields above this
  /// comment.
  utils::statistics::Entry statistics_holder_;
};

}  // namespace kafka

USERVER_NAMESPACE_END
