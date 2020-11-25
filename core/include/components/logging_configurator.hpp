#pragma once

#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <components/impl/component_base.hpp>
#include <taxi_config/config_fwd.hpp>
#include <utils/async_event_channel.hpp>

namespace components {

/// Helper component to configure logging.
///
/// Can not move the functionality into Trace or Logger components because that
/// inroduces circular dependency between Logger and TaxiConfig.
class LoggingConfigurator final : public impl::ComponentBase {
 public:
  static constexpr auto kName = "logging-configurator";

  LoggingConfigurator(const ComponentConfig& config,
                      const ComponentContext& context);

 private:
  void OnConfigUpdate(const std::shared_ptr<const taxi_config::Config>& config);

  utils::AsyncEventSubscriberScope config_subscription_;
};

}  // namespace components
