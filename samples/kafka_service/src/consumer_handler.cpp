#include <consumer_handler.hpp>

#include <userver/kafka/consumer_component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>

#include <consume.hpp>

namespace kafka_sample {

/// [Kafka service sample - consumer usage]
ConsumerHandler::ConsumerHandler(const components::ComponentConfig& config,
                                 const components::ComponentContext& context)
    : components::ComponentBase{config, context},
      consumer_{
          context.FindComponent<kafka::ConsumerComponent>().GetConsumer()} {
  consumer_.Start([this](kafka::MessageBatchView messages) {
    Consume(messages);
    consumer_.AsyncCommit();
  });
}
/// [Kafka service sample - consumer usage]

}  // namespace kafka_sample
