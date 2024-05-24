#pragma once

#include <userver/kafka/consumer_scope.hpp>

#include <userver/components/loggable_component_base.hpp>
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
/// auto_offset_reset                  | action to take when there is no initial offset in offset store | --
/// max_batch_size                     | maximum batch size for one callback call | --
/// env_pod_name                       | environment variable to substitute `{pod_name}` substring in `group_id` | none
/// security_protocol                  | protocol used to communicate with brokers | --
/// sasl_mechanisms                    | SASL mechanism to use for authentication | none
/// ssl_ca_location                    | File or directory path to CA certificate(s) for verifying the broker's key | 300000
/// topic_metadata_refresh_interval_ms | period of time in milliseconds at which topic and broker metadata is refreshed | 900000
/// metadata_max_age_ms                | metadata cache max age | none

// clang-format on

class ConsumerComponent final : public components::LoggableComponentBase {
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
