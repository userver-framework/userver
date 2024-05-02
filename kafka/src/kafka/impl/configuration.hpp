#pragma once

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/logging/log_extra.hpp>

#include <kafka/impl/broker_secrets.hpp>
#include <kafka/impl/producer_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

class Configuration {
 public:
  Configuration(const components::ComponentConfig& config,
                const components::ComponentContext& context);

  std::unique_ptr<ProducerImpl> MakeProducerImpl(
      const logging::LogExtra& log_tags) const;

 private:
  const components::ComponentConfig config_;
  const impl::BrokerSecrets broker_secrets_;
};

}  // namespace kafka

USERVER_NAMESPACE_END
