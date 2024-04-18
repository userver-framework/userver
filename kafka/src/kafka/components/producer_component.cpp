#include <userver/kafka/components/producer_component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/storages/secdist/component.hpp>

#include <cppkafka/configuration.h>

#include <kafka/configuration.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

ProducerComponent::ProducerComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : components::LoggableComponentBase(config, context),
      producer_(
          MakeProducerConfiguration(context.FindComponent<components::Secdist>()
                                        .Get()
                                        .Get<BrokerSecrets>(),
                                    config),
          config.Name(), context.GetTaskProcessor("producer-task-processor"),
          config["is_testsuite_mode"].As<bool>(),
          context.FindComponent<components::StatisticsStorage>().GetStorage()) {
}

ProducerComponent::~ProducerComponent() = default;

Producer& ProducerComponent::GetProducer() { return producer_; }

}  // namespace kafka

USERVER_NAMESPACE_END
