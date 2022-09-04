#pragma once

/// @file userver/urabbitmq/component.hpp
/// @brief @copybrief components::RabbitMQ

#include <memory>

#include <userver/components/loggable_component_base.hpp>

#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::dns {
class Component;
}

namespace urabbitmq {
class Client;
}

namespace components {

// clang-format off
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
/// ## Static options:
/// Name                    | Description                                                          | Default value
/// ----------------------- | -------------------------------------------------------------------- | ---------------
/// secdist_alias           | name of the key in secdist config                                    | components name
/// min_pool_size           | minimum connections pool size (per host)                             | 5
/// max_pool_size           | maximum connections pool size (per host, consumers excluded)         | 10
/// max_in_flight_requests  | per-connection limit for requests awaiting response from the broker  | 5
/// use_secure_connection   | whether to use TLS for connections                                   | true
///
// clang-format on

class RabbitMQ : public LoggableComponentBase {
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
  utils::statistics::Entry statistics_holder_;
};

template <>
inline constexpr bool kHasValidate<RabbitMQ> = true;

}  // namespace components

USERVER_NAMESPACE_END
