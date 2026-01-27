#include <userver/kafka/consumer_component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/kafka/impl/broker_secrets.hpp>
#include <userver/kafka/impl/configuration.hpp>
#include <userver/kafka/impl/consumer.hpp>
#include <userver/logging/level_serialization.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/kafka/consumer_component.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace kafka {

ConsumerComponent::ConsumerComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : components::ComponentBase(config, context),
      consumer_(
          config.Name(),
          config["topics"].As<std::vector<std::string>>(),
          context.GetTaskProcessor("consumer-task-processor"),
          context.GetTaskProcessor("consumer-blocking-task-processor"),
          context.GetTaskProcessor("main-task-processor"),
          config.As<impl::ConsumerConfiguration>(),
          context.FindComponent<components::Secdist>()
              .Get()
              .Get<impl::BrokerSecrets>()
              .GetSecretByComponentName(config.Name()),
          [&config] {
              impl::ConsumerExecutionParams params{};
              params.max_batch_size = config["max_batch_size"].As<std::size_t>(params.max_batch_size);
              params.poll_timeout = config["poll_timeout"].As<std::chrono::milliseconds>(params.poll_timeout);
              params.max_callback_duration =
                  config["max_callback_duration"].As<std::chrono::milliseconds>(params.max_callback_duration);
              params.restart_after_failure_delay =
                  config["restart_after_failure_delay"].As<std::chrono::milliseconds>(params.restart_after_failure_delay
                  );
              params.message_key_log_format = config["message_key_log_format"].As<impl::MessageKeyLogFormat>();
              params
                  .debug_info_log_level = config["debug_info_log_level"].As<logging::Level>(params.debug_info_log_level
              );
              params.operation_log_level = config["operation_log_level"].As<logging::Level>(params.operation_log_level);

              return params;
          }()
      )
{
    auto& storage = context.FindComponent<components::StatisticsStorage>().GetStorage();

    statistics_holder_ = storage.RegisterWriter("kafka_consumer", [this](utils::statistics::Writer& writer) {
        consumer_->DumpMetric(writer);
    });
}

ConsumerComponent::~ConsumerComponent() { statistics_holder_.Unregister(); }

ConsumerScope ConsumerComponent::GetConsumer() { return consumer_->MakeConsumerScope(); }

yaml_config::Schema ConsumerComponent::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<components::ComponentBase>("src/kafka/consumer_component.yaml");
}

}  // namespace kafka

USERVER_NAMESPACE_END
