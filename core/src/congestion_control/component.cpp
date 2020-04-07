#include <congestion_control/component.hpp>

#include <congestion_control/watchdog.hpp>
#include <server/component.hpp>
#include <server/congestion_control/limiter.hpp>
#include <server/congestion_control/sensor.hpp>
#include <taxi_config/storage/component.hpp>
#include <taxi_config/value.hpp>
#include <utils/async_event_channel.hpp>

namespace congestion_control {

namespace {

const auto kServerControllerName = "server-main-tp-cc";

struct RpsCcConfig {
  using DocsMap = taxi_config::DocsMap;

  explicit RpsCcConfig(const DocsMap& docs_map)
      : policy(MakePolicy(docs_map.Get("USERVER_RPS_CCONTROL"))),
        is_enabled(docs_map.Get("USERVER_RPS_CCONTROL_ENABLED").As<bool>()) {}

  Policy policy;
  bool is_enabled;
};
}  // namespace

struct Component::Impl {
  server::congestion_control::Sensor server_sensor;
  server::congestion_control::Limiter server_limiter;
  Controller server_controller;

  // must go after all sensors/limiters
  Watchdog wd;
  utils::AsyncEventSubscriberScope config_subscription;

  Impl(server::Server& server, engine::TaskProcessor& tp)
      : server_sensor(server, tp),
        server_limiter(server),
        server_controller(kServerControllerName, {}) {}
};

Component::Component(const components::ComponentConfig& config,
                     const components::ComponentContext& context)
    : components::LoggableComponentBase(config, context),
      pimpl_(context.FindComponent<components::Server>().GetServer(),
             engine::current_task::GetTaskProcessor())

{
  pimpl_->wd.Register({pimpl_->server_sensor, pimpl_->server_limiter,
                       pimpl_->server_controller});

  auto& taxi_config = context.FindComponent<components::TaxiConfig>();
  pimpl_->config_subscription =
      taxi_config.AddListener(this, kName, &Component::OnConfigUpdate);
}

Component::~Component() = default;

void Component::OnConfigUpdate(
    const std::shared_ptr<const taxi_config::Config>& cfg) {
  const auto& rps_cc = cfg->Get<RpsCcConfig>();
  pimpl_->server_controller.SetPolicy(rps_cc.policy);
  pimpl_->server_controller.SetEnabled(rps_cc.is_enabled);
}

}  // namespace congestion_control
