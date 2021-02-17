#pragma once

#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <components/impl/component_base.hpp>
#include <taxi_config/config_fwd.hpp>
#include <utils/async_event_channel.hpp>

namespace components {

// clang-format off

/// @ingroup userver_components
///
/// @brief Helper component to configure logging.
///
/// The functionality is not in Trace or Logger components because that
/// inroduces circular dependency between Logger and TaxiConfig.
///
/// Component must be configured in service config.
///
/// ## Available options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// limited-logging-enable | set to true to make LOG_LIMITED drop repeated logs | -
/// limited-logging-interval | utils::StringToDuration suitable duration string to group repeated logs into one message | -
///
/// ## Config example:
///
/// @snippet components/manager_config_test.cpp Sample logging configurator

// clang-format on
class LoggingConfigurator final : public impl::ComponentBase {
 public:
  static constexpr auto kName = "logging-configurator";

  LoggingConfigurator(const ComponentConfig& config,
                      const ComponentContext& context);

 private:
  void OnConfigUpdate(const std::shared_ptr<const taxi_config::Config>& config);

  utils::AsyncEventSubscriberScope config_subscription_;
};

/// }@

}  // namespace components
