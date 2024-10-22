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
/// @snippet samples/kafka_service/static_config.yaml  Kafka service sample - consumer static config
///
/// ## Secdist format
///
/// A Kafka alias in secdist is described as a JSON object
/// `kafka_settings`, containing credentials of Kafka brokers.
///
/// @snippet samples/kafka_service/testsuite/conftest.py  Kafka service sample - secdist
///
/// ## Static options:
/// Name                               | Description                                      | Default value
/// ---------------------------------- | ------------------------------------------------ | ---------------
/// client_id                          | Client identifier. May be an arbitrary string | userver
/// group_id                           | consumer group id (name) | --
/// topics                             | list of topics consumer subscribes | --
/// max_batch_size                     | maximum number of messages consumer waits for new message before calling a callback | 1
/// poll_timeout                       | maximum amount of time consumer waits for messages for new messages before calling a callback | 1s
/// max_callback_duration              | duration user callback must fit not to be kicked from the consumer group | 5m
/// restart_after_failure_delay        | time consumer suspends execution if user-callback fails | 10s
/// auto_offset_reset                  | action to take when there is no initial offset in offset store | smallest
/// env_pod_name                       | environment variable to substitute `{pod_name}` substring in `group_id` | none
/// security_protocol                  | protocol used to communicate with brokers | --
/// sasl_mechanisms                    | SASL mechanism to use for authentication | none
/// ssl_ca_location                    | File or directory path to CA certificate(s) for verifying the broker's key | none
/// topic_metadata_refresh_interval    | period of time at which topic and broker metadata is refreshed | 5m
/// metadata_max_age                   | metadata cache max age | 15
/// rd_kafka_custom_options            | a map of librdkafka library additional options | '{}'

// clang-format on

class ConsumerComponent final : public components::ComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of kafka::ConsumerComponent component
    static constexpr std::string_view kName{"kafka-consumer"};

    ConsumerComponent(const components::ComponentConfig& config, const components::ComponentContext& context);
    ~ConsumerComponent() override;

    /// @brief Returns consumer instance.
    /// @see kafka::ConsumerScope
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
