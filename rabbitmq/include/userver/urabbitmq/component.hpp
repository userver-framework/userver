#pragma once

#include <memory>

#include <userver/components/loggable_component_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::dns {
class Component;
}

namespace urabbitmq {
class Client;
}

namespace components {

class RabbitMQ : public LoggableComponentBase {
 public:
  RabbitMQ(const ComponentConfig& config, const ComponentContext& context);
  ~RabbitMQ() override;

  std::shared_ptr<urabbitmq::Client> GetClient() const;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  clients::dns::Component& dns_;

  std::shared_ptr<urabbitmq::Client> client_;
};

template <>
inline constexpr bool kHasValidate<RabbitMQ> = true;

}  // namespace components

USERVER_NAMESPACE_END