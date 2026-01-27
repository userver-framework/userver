#include <userver/kafka/producer_component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/kafka/impl/broker_secrets.hpp>
#include <userver/kafka/impl/configuration.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/utils/statistics/writer.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/kafka/producer_component.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace kafka {

ProducerComponent::ProducerComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : components::ComponentBase(config, context),
      producer_(
          config.Name(),
          context.GetTaskProcessor("producer-task-processor"),
          config.As<impl::ProducerConfiguration>(),
          context.FindComponent<components::Secdist>()
              .Get()
              .Get<impl::BrokerSecrets>()
              .GetSecretByComponentName(config.Name())
      )
{
    auto& storage = context.FindComponent<components::StatisticsStorage>().GetStorage();

    statistics_holder_ = storage.RegisterWriter("kafka_producer", [this](utils::statistics::Writer& writer) {
        producer_.DumpMetric(writer);
    });
}

ProducerComponent::~ProducerComponent() { statistics_holder_.Unregister(); }

const Producer& ProducerComponent::GetProducer() { return producer_; }

yaml_config::Schema ProducerComponent::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<components::ComponentBase>("src/kafka/producer_component.yaml");
}

}  // namespace kafka

USERVER_NAMESPACE_END
