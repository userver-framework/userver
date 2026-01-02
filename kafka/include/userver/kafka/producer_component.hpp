#pragma once

#include <string_view>

#include <userver/kafka/producer.hpp>

#include <userver/components/component_base.hpp>
#include <userver/utils/statistics/entry.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

/// @ingroup userver_components
///
/// @brief Apache Kafka Producer client component.
///
/// ## Static configuration example:
///
/// @snippet samples/kafka_service/static_config.yaml  Kafka service sample - producer static config
///
/// ## Secdist format
///
/// A Kafka alias in secdist is described as a JSON object
/// `kafka_settings`, containing credentials of Kafka brokers.
///
/// @snippet samples/kafka_service/testsuite/conftest.py  Kafka service sample - secdist
///
/// ## Static options of kafka::ProducerComponent:
/// @include{doc} scripts/docs/en/components_schema/kafka/src/kafka/producer_component.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
class ProducerComponent final : public components::ComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of kafka::ProducerComponent component
    static constexpr std::string_view kName{"kafka-producer"};

    ProducerComponent(const components::ComponentConfig& config, const components::ComponentContext& context);
    ~ProducerComponent() override;

    /// @brief Returns a producer instance reference.
    /// @see kafka::Producer
    const Producer& GetProducer();

    static yaml_config::Schema GetStaticConfigSchema();

private:
    Producer producer_;

    /// @note Subscriptions must be the last fields! Add new fields above this
    /// comment.
    utils::statistics::Entry statistics_holder_;
};

}  // namespace kafka

USERVER_NAMESPACE_END
