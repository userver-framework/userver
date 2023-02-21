#include <userver/congestion_control/component.hpp>

#include <congestion_control/watchdog.hpp>
#include <server/congestion_control/limiter.hpp>
#include <server/congestion_control/sensor.hpp>
#include <userver/congestion_control/config.hpp>

#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/concurrent/async_event_channel.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/hostinfo/cpu_limit.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/component.hpp>
#include <userver/server/server.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace congestion_control {

namespace {

const auto kServerControllerName = "server-main-tp-cc";

void FormatStats(const Controller& c, size_t activated_factor,
                 utils::statistics::Writer& writer) {
  writer["is-enabled"] = c.IsEnabled() ? 1 : 0;

  auto limit = c.GetLimit();
  writer["is-activated"] =
      (limit.load_limit &&
       limit.current_load * activated_factor < *limit.load_limit)
          ? 1
          : 0;
  if (limit.load_limit) {
    writer["limit"] = *limit.load_limit;
  }

  const auto& stats = c.GetStats();
  {
    auto builder_states = writer["states"];
    builder_states["no-limit"] = stats.no_limit;
    builder_states["not-overloaded-no-pressure"] =
        stats.not_overload_no_pressure;
    builder_states["not-overloaded-under-pressure"] =
        stats.not_overload_pressure;
    builder_states["overloaded-no-pressure"] = stats.overload_no_pressure;
    builder_states["overloaded-under-pressure"] = stats.overload_pressure;
  }

  auto diff = std::chrono::steady_clock::now().time_since_epoch() -
              stats.last_overload_pressure.load();
  writer["time-from-last-overloaded-under-pressure-secs"] =
      std::chrono::duration_cast<std::chrono::seconds>(diff).count();
  writer["current-state"] = stats.current_state;
}

}  // namespace

struct Component::Impl {
  dynamic_config::Source dynamic_config;
  server::Server& server;
  server::congestion_control::Sensor server_sensor;
  server::congestion_control::Limiter server_limiter;
  Controller server_controller;

  utils::statistics::Entry statistics_holder;

  // must go after all sensors/limiters
  Watchdog wd;
  concurrent::AsyncEventSubscriberScope config_subscription;
  std::atomic<bool> fake_mode;
  std::atomic<bool> force_disabled{false};
  std::atomic<size_t> last_activate_factor{1};

  Impl(dynamic_config::Source dynamic_config, server::Server& server,
       engine::TaskProcessor& tp, bool fake_mode)
      : dynamic_config(dynamic_config),
        server(server),
        server_sensor(server, tp),
        server_limiter(server),
        server_controller(kServerControllerName, dynamic_config),
        fake_mode(fake_mode) {}
};

Component::Component(const components::ComponentConfig& config,
                     const components::ComponentContext& context)
    : components::LoggableComponentBase(config, context),
      pimpl_(context.FindComponent<components::DynamicConfig>().GetSource(),
             context.FindComponent<components::Server>().GetServer(),
             engine::current_task::GetTaskProcessor(),
             config["fake-mode"].As<bool>(false)) {
  auto min_threads = config["min-cpu"].As<size_t>(1);
  auto only_rtc = config["only-rtc"].As<bool>(true);

  pimpl_->server.SetRpsRatelimitStatusCode(
      static_cast<server::http::HttpStatus>(
          config["status-code"].As<int>(429)));

  if (!pimpl_->fake_mode && only_rtc && !hostinfo::IsInRtc()) {
    LOG_WARNING() << "Started outside of RTC, forcing fake-mode";
    pimpl_->fake_mode = true;
  }

  auto cpu_limit = hostinfo::CpuLimit().value_or(0);
  if (!pimpl_->fake_mode && cpu_limit < min_threads) {
    LOG_WARNING() << fmt::format(
        "current cpu limit ({}) less that minimum limit ({}) "
        "for Congestion Control, forcing fake-mode",
        cpu_limit, min_threads);
    pimpl_->fake_mode = true;
  }

  if (pimpl_->fake_mode) {
    LOG_WARNING() << "congestion_control is started in fake-mode, no RPS limit "
                     "is enforced";
  }

  pimpl_->wd.Register({pimpl_->server_sensor, pimpl_->server_limiter,
                       pimpl_->server_controller});

  pimpl_->config_subscription = pimpl_->dynamic_config.UpdateAndListen(
      this, kName, &Component::OnConfigUpdate);

  auto& storage =
      context.FindComponent<components::StatisticsStorage>().GetStorage();
  pimpl_->statistics_holder = storage.RegisterWriter(
      std::string{kName},
      [this](utils::statistics::Writer& writer) { ExtendWriter(writer); });
}

Component::~Component() {
  pimpl_->config_subscription.Unsubscribe();
  pimpl_->statistics_holder.Unregister();
}

void Component::OnConfigUpdate(const dynamic_config::Snapshot& cfg) {
  const auto& conf = cfg[impl::kRpsCcConfig];
  const bool is_enabled_dynamic = conf.is_enabled;
  pimpl_->last_activate_factor = conf.activate_factor;

  bool enabled = !pimpl_->fake_mode.load() && !pimpl_->force_disabled.load();
  if (enabled && !is_enabled_dynamic) {
    LOG_INFO() << "Congestion control is explicitly disabled in "
                  "USERVER_RPS_CCONTROL_ENABLED config";
    enabled = false;
  }
  pimpl_->server_controller.SetEnabled(enabled);
}

void Component::OnAllComponentsLoaded() {
  LOG_DEBUG() << "Found " << pimpl_->server.GetRegisteredHandlersCount()
              << " registered HTTP handlers";
  if (pimpl_->server.GetRegisteredHandlersCount() == 0) {
    pimpl_->force_disabled = true;
    LOG_WARNING() << "No HTTP handlers registered, disabling";

    // apply fake_mode
    OnConfigUpdate(pimpl_->dynamic_config.GetSnapshot());
  }
}

void Component::OnAllComponentsAreStopping() { pimpl_->wd.Stop(); }

void Component::ExtendWriter(utils::statistics::Writer& writer) {
  if (!pimpl_->force_disabled) {
    auto rps = writer["rps"];
    FormatStats(pimpl_->server_controller, pimpl_->last_activate_factor, rps);
  }
}

yaml_config::Schema Component::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<components::LoggableComponentBase>(R"(
type: object
description: Component to limit too active requests, also known as CC.
additionalProperties: false
properties:
    fake-mode:
        type: boolean
        description: if set, an actual throttling is skipped, but FSM is still working and producing informational logs
        defaultDescription: false
    min-cpu:
        type: integer
        description: force fake-mode if the current cpu number is less than the specified value
        defaultDescription: 1
    only-rtc:
        type: boolean
        description: if set to true and hostinfo::IsInRtc() returns false then forces the fake-mode
        defaultDescription: true
    status-code:
        type: integer
        description: HTTP status code for ratelimited responses
        defaultDescription: 429
)");
}

}  // namespace congestion_control

USERVER_NAMESPACE_END
