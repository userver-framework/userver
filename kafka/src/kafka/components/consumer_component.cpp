#include <userver/kafka/components/consumer_component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/storages/secdist/component.hpp>

#include <cppkafka/configuration.h>

#include <kafka/configuration.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

ConsumerComponent::ConsumerComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : components::LoggableComponentBase(config, context),
      consumer_(MakeConsumerConfiguration(
                    context.FindComponent<::components::Secdist>()
                        .Get()
                        .Get<BrokerSecrets>(),
                    config),
                config.Name(), config["topics"].As<std::vector<std::string>>(),
                config["max_batch_size"].As<size_t>(),
                context.GetTaskProcessor("consumer-task-processor"),
                context.GetTaskProcessor("main-task-processor"),
                config["is_testsuite_mode"].As<bool>(),
                context.FindComponent<::components::StatisticsStorage>()
                    .GetStorage()) {}

ConsumerComponent::~ConsumerComponent() = default;

Consumer& ConsumerComponent::GetConsumer() { return consumer_; }

}  // namespace kafka

USERVER_NAMESPACE_END
