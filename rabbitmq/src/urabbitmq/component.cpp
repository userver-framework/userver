#include <userver/urabbitmq/component.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/urabbitmq/client.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

std::string GetSecdistAlias(const components::ComponentConfig& config) {
  return config.HasMember("secdist_alias")
             ? config["secdist_alias"].As<std::string>()
             : config.Name();
}

}  // namespace

RabbitMQ::RabbitMQ(const ComponentConfig& config,
                   const ComponentContext& context)
    : LoggableComponentBase{config, context},
      dns_{context.FindComponent<clients::dns::Component>()} {
  const auto& secdist = context.FindComponent<Secdist>().Get();
  const auto& settings_multi = secdist.Get<urabbitmq::RabbitEndpointsMulti>();
  const auto& endpoints = settings_multi.Get(GetSecdistAlias(config));

  const urabbitmq::ClientSettings settings{config, endpoints};
  client_ = urabbitmq::Client::Create(dns_.GetResolver(), settings);

  auto& statistics_storage =
      context.FindComponent<components::StatisticsStorage>();
  statistics_holder_ = statistics_storage.GetStorage().RegisterWriter(
      "rabbitmq." + config.Name(), [this](utils::statistics::Writer& writer) {
        return client_->WriteStatistics(writer);
      });
}

RabbitMQ::~RabbitMQ() { statistics_holder_.Unregister(); }

std::shared_ptr<urabbitmq::Client> RabbitMQ::GetClient() const {
  return client_;
}

yaml_config::Schema RabbitMQ::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<LoggableComponentBase>(R"(
# yaml
type: object
description: RabbitMQ client component
additionalProperties: false
properties:
    secdist_alias:
        type: string
        description: name of the key in secdist config
    min_pool_size:
        type: integer
        description: minimum connections pool size (per host)
        defaultDescription: 5
    max_pool_size:
        type: integer
        description: |
          maximum connections pool size (per host, consumers excluded)
        defaultDescription: 10
    max_in_flight_requests:
        type: integer
        description: |
          per-connection limit for requests awaiting response from the broker
        defaultDescription: 5
    use_secure_connection:
        type: boolean
        description: whether to use TLS for connections
        defaultDescription: true
)");
}

}  // namespace components

USERVER_NAMESPACE_END
