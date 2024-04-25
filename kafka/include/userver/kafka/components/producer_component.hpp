#pragma once

#include <userver/components/loggable_component_base.hpp>

#include <userver/kafka/producer.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

class ProducerComponent final : public components::LoggableComponentBase {
 public:
  static constexpr auto kName = "kafka-producer";

  ProducerComponent(const components::ComponentConfig& config,
                    const components::ComponentContext& context);
  ~ProducerComponent() override;

  Producer& GetProducer();

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  Producer producer_;
};

}  // namespace kafka

USERVER_NAMESPACE_END
