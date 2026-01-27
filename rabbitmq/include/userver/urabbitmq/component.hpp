#pragma once

/// @file userver/urabbitmq/component.hpp
/// @brief @copybrief components::RabbitMQ

#include <memory>

#include <userver/components/component_base.hpp>
#include <userver/utils/statistics/entry.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::dns {
class Component;
}

namespace urabbitmq {
class Client;
}

namespace components {

/// @ingroup userver_components
///
/// @brief RabbitMQ (AMQP 0.9.1) client component
///
/// Provides access to a RabbitMQ cluster.
///
/// ## Static configuration example:
///
/// @snippet samples/rabbitmq_service/static_config.yaml  RabbitMQ service sample - static config
///
/// If the component is configured with an secdist_alias, it will lookup
/// connection data in secdist.json via secdist_alias value, otherwise via
/// components name.
///
/// ## Secdist format
///
/// A RabbitMQ alias in secdist is described as a JSON object
/// 'rabbitmq_settings', containing descriptions of RabbitMQ clusters.
///
/// @snippet samples/rabbitmq_service/tests/conftest.py  RabbitMQ service sample - secdist
///
/// ## Static options of components::RabbitMQ :
/// @include{doc} scripts/docs/en/components_schema/rabbitmq/src/urabbitmq/component.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
class RabbitMQ final : public ComponentBase {
public:
    /// Component constructor
    RabbitMQ(const ComponentConfig& config, const ComponentContext& context);
    /// Component destructor
    ~RabbitMQ() override;

    /// Cluster accessor
    std::shared_ptr<urabbitmq::Client> GetClient() const;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    clients::dns::Component& dns_;
    std::shared_ptr<urabbitmq::Client> client_;

    // Must be the last field
    utils::statistics::Entry statistics_holder_;
};

template <>
inline constexpr bool kHasValidate<RabbitMQ> = true;

}  // namespace components

USERVER_NAMESPACE_END
