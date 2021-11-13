#include <userver/congestion_control/component.hpp>

#include <congestion_control/watchdog.hpp>
#include <server/congestion_control/limiter.hpp>
#include <server/congestion_control/sensor.hpp>

#include <userver/concurrent/async_event_channel.hpp>
#include <userver/hostinfo/cpu_limit.hpp>
#include <userver/server/component.hpp>
#include <userver/server/server.hpp>
#include <userver/taxi_config/storage/component.hpp>
#include <userver/taxi_config/value.hpp>

USERVER_NAMESPACE_BEGIN

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

formats::json::Value FormatStats(const Controller& c) {
  formats::json::ValueBuilder builder;
  builder["is-enabled"] = c.IsEnabled() ? 1 : 0;

  auto limit = c.GetLimit();
  builder["is-activated"] = limit.load_limit ? 1 : 0;
  if (limit.load_limit) {
    builder["limit"] = *limit.load_limit;
  }

  const auto& stats = c.GetStats();
  formats::json::ValueBuilder builder_states;
  builder_states["no-limit"] = stats.no_limit.load();
  builder_states["not-overloaded-no-pressure"] =
      stats.not_overload_no_pressure.load();
  builder_states["not-overloaded-under-pressure"] =
      stats.not_overload_pressure.load();
  builder_states["overloaded-no-pressure"] = stats.overload_no_pressure.load();
  builder_states["overloaded-under-pressure"] = stats.overload_pressure.load();

  auto diff = std::chrono::steady_clock::now().time_since_epoch() -
              stats.last_overload_pressure.load();
  builder["time-from-last-overloaded-under-pressure-secs"] =
      std::chrono::duration_cast<std::chrono::seconds>(diff).count();
  builder["states"] = builder_states.ExtractValue();
  builder["current-state"] = stats.current_state.load();

  return builder.ExtractValue();
}

}  // namespace

struct Component::Impl {
  taxi_config::Source taxi_config;
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

  Impl(taxi_config::Source taxi_config, server::Server& server,
       engine::TaskProcessor& tp, bool fake_mode)
      : taxi_config(taxi_config),
        server(server),
        server_sensor(server, tp),
        server_limiter(server),
        server_controller(kServerControllerName, {}),
        fake_mode(fake_mode) {}
};

Component::Component(const components::ComponentConfig& config,
                     const components::ComponentContext& context)
    : components::LoggableComponentBase(config, context),
      pimpl_(context.FindComponent<components::TaxiConfig>().GetSource(),
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

  pimpl_->config_subscription = pimpl_->taxi_config.UpdateAndListen(
      this, kName, &Component::OnConfigUpdate);

  auto& storage =
      context.FindComponent<components::StatisticsStorage>().GetStorage();
  pimpl_->statistics_holder = storage.RegisterExtender(
      std::string{kName},
      std::bind(&Component::ExtendStatistics, this, std::placeholders::_1));
}

Component::~Component() = default;

void Component::OnConfigUpdate(const taxi_config::Snapshot& cfg) {
  const auto& rps_cc = cfg.Get<RpsCcConfig>();
  pimpl_->server_controller.SetPolicy(rps_cc.policy);

  bool enabled = !pimpl_->fake_mode.load() && !pimpl_->force_disabled.load();
  if (enabled && !rps_cc.is_enabled) {
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
    OnConfigUpdate(pimpl_->taxi_config.GetSnapshot());
  }
}

void Component::OnAllComponentsAreStopping() { pimpl_->wd.Stop(); }

formats::json::Value Component::ExtendStatistics(
    const utils::statistics::StatisticsRequest& /*request*/) {
  if (pimpl_->force_disabled) return {};

  formats::json::ValueBuilder builder;
  builder["rps"] = FormatStats(pimpl_->server_controller);
  return builder.ExtractValue();
}

}  // namespace congestion_control

USERVER_NAMESPACE_END
