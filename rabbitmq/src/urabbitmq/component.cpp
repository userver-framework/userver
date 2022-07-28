#include <userver/urabbitmq/component.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/urabbitmq/client.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

RabbitMQ::RabbitMQ(const ComponentConfig& config,
                   const ComponentContext& context)
    : LoggableComponentBase{config, context},
      dns_{context.FindComponent<clients::dns::Component>()},
      client_{urabbitmq::Client::Create(dns_.GetResolver(), {})} {}

RabbitMQ::~RabbitMQ() = default;

std::shared_ptr<urabbitmq::Client> RabbitMQ::GetClient() const {
  return client_;
}

yaml_config::Schema RabbitMQ::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<LoggableComponentBase>(R"(
type: object
description: RabbitMQ client component
additionalProperties: false
properties:
    secdist_alias:
        type: string
        description: name of the key in secdist config
    ev_pool_type:
        type: string
        description: |
          whether to use the default framework ev-pool (shared)
          or create a new one (owned)
        defaultDescription: owned
    thread_count:
        type: integer
        description: |
          how many ev-threads should component create,
          ignored with ev_pool_type: shared
        defaultDescription: 2
    connections_per_thread:
        type: integer
        description: how many connections should component create per ev-thread
        defaultDescription: 1
    channels_per_connection:
        type: integer
        description: |
          how many channels of each supported type should component create
          per connection
        defaultDescription: 10
    use_secure_connection:
        type: boolean
        description: whether to use TLS for connections
        defaultDescription: false
)");
}

}  // namespace components

USERVER_NAMESPACE_END