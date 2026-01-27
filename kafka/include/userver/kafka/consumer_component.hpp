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
/// ## Static options of kafka::ConsumerComponent :
/// @include{doc} scripts/docs/en/components_schema/kafka/src/kafka/consumer_component.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
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
