#include <userver/urabbitmq/component.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/urabbitmq/client.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/urabbitmq/component.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

std::string GetSecdistAlias(const components::ComponentConfig& config) {
    return config["secdist_alias"].As<std::string>(config.Name());
}

}  // namespace

RabbitMQ::RabbitMQ(const ComponentConfig& config, const ComponentContext& context)
    : ComponentBase{config, context},
      dns_{context.FindComponent<clients::dns::Component>()}
{
    const auto& secdist = context.FindComponent<Secdist>().Get();
    const auto& settings_multi = secdist.Get<urabbitmq::RabbitEndpointsMulti>();
    const auto& endpoints = settings_multi.Get(GetSecdistAlias(config));

    const urabbitmq::ClientSettings settings{config, endpoints};
    client_ = urabbitmq::Client::Create(dns_.GetResolver(), settings);

    auto& statistics_storage = context.FindComponent<components::StatisticsStorage>();
    statistics_holder_ =
        statistics_storage.GetStorage()
            .RegisterWriter("rabbitmq." + config.Name(), [this](utils::statistics::Writer& writer) {
                return client_->WriteStatistics(writer);
            });
}

RabbitMQ::~RabbitMQ() { statistics_holder_.Unregister(); }

std::shared_ptr<urabbitmq::Client> RabbitMQ::GetClient() const { return client_; }

yaml_config::Schema RabbitMQ::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<ComponentBase>("src/urabbitmq/component.yaml");
}

}  // namespace components

USERVER_NAMESPACE_END
