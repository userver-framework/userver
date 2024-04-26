#pragma once

#include <userver/components/loggable_component_base.hpp>

#include <userver/kafka/consumer.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

class ConsumerComponent final : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "kafka-consumer";

  ConsumerComponent(const components::ComponentConfig& config,
                    const components::ComponentContext& context);
  ~ConsumerComponent() override;

  Consumer& GetConsumer();

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  Consumer consumer_;
};

}  // namespace kafka

USERVER_NAMESPACE_END
