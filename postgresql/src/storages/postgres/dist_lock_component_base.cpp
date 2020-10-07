#include <components/statistics_storage.hpp>
#include <dist_lock/dist_lock_settings.hpp>
#include <storages/postgres/component.hpp>
#include <storages/postgres/dist_lock_component_base.hpp>
#include <utils/statistics/metadata.hpp>

namespace storages::postgres {

DistLockComponentBase::DistLockComponentBase(
    const components::ComponentConfig& component_config,
    const components::ComponentContext& component_context)
    : components::LoggableComponentBase(component_config, component_context) {
  auto cluster = component_context
                     .FindComponent<components::Postgres>(
                         component_config.ParseString("cluster"))
                     .GetCluster();
  auto table = component_config.ParseString("table");
  auto lock_name = component_config.ParseString("lockname");

  auto ttl = component_config.ParseDuration("lock-ttl");
  auto pg_timeout = component_config.ParseDuration("pg-timeout");
  auto optional_restart_delay =
      component_config.ParseOptionalDuration("restart-delay");
  const auto prolong_ratio = 10;

  if (pg_timeout >= ttl / 2)
    throw std::runtime_error("pg-timeout must be less than lock-ttl / 2");

  dist_lock::DistLockSettings settings{ttl / prolong_ratio, ttl / prolong_ratio,
                                       ttl, pg_timeout};
  if (optional_restart_delay) {
    settings.worker_func_restart_delay = *optional_restart_delay;
  }

  auto strategy = std::make_shared<DistLockStrategy>(std::move(cluster), table,
                                                     lock_name, settings);

  worker_ = std::make_unique<dist_lock::DistLockedWorker>(
      lock_name, [this]() { DoWork(); }, std::move(strategy), settings);

  autostart_ = component_config.ParseBool("autostart", false);

  auto& statistics_storage =
      component_context.FindComponent<components::StatisticsStorage>();
  statistics_holder_ = statistics_storage.GetStorage().RegisterExtender(
      "distlock." + component_config.Name(),
      [this](const ::utils::statistics::StatisticsRequest&) {
        return worker_->GetStatisticsJson();
      });
}

DistLockComponentBase::~DistLockComponentBase() {
  statistics_holder_.Unregister();
}

dist_lock::DistLockedWorker& DistLockComponentBase::GetWorker() {
  return *worker_;
}

void DistLockComponentBase::AutostartDistLock() {
  if (autostart_) worker_->Start();
}

void DistLockComponentBase::StopDistLock() { worker_->Stop(); }

}  // namespace storages::postgres
