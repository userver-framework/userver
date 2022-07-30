#pragma once

/// @file userver/urabbitmq/component.hpp
/// @brief @copybrief components::RabbitMQ

#include <memory>

#include <userver/components/loggable_component_base.hpp>

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
/// @snippet TODO
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
/// @snippet TODO
///
/// ## Static options:
/// Name                    | Description                                                                        | Default value
/// ----------------------- | ---------------------------------------------------------------------------------- | ---------------
/// secdist_alias           | name of the key in secdist config                                                  | components name
/// ev_pool_type            | whether to use the default framework ev-pool (shared) or create a new one (owned)  | owned
/// thread_count            | how many ev-threads should component create, ignored with ev_pool_type: shared     | 2
/// connections_per_thread  | how many connections should component create per ev-thread                         | 1
/// channels_per_connection | how many channels of each supported type should component create per connection    | 10
/// use_secure_connection   | whether to use TLS for connections                                                 | true
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
};

template <>
inline constexpr bool kHasValidate<RabbitMQ> = true;

}  // namespace components

USERVER_NAMESPACE_END