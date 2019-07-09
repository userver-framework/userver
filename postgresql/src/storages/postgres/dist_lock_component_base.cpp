#include <components/statistics_storage.hpp>
#include <storages/postgres/component.hpp>
#include <storages/postgres/dist_lock_component_base.hpp>
#include <utils/statistics/metadata.hpp>

namespace storages {
namespace postgres {

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

  auto interval = component_config.ParseDuration("prolong-interval");
  auto pg_timeout = component_config.ParseDuration("pg-timeout");
  const auto prolong_ratio = 10;

  if (pg_timeout >= interval / 2)
    throw std::runtime_error(
        "pg-timeout must be less than prolong-interval / 2");

  DistLockedTaskSettings settings{
      interval / prolong_ratio, interval / prolong_ratio, interval, pg_timeout};

  task_ = std::make_unique<DistLockedTask>(std::move(cluster), table, lock_name,
                                           [this]() { DoWork(); }, settings);

  autostart_ = component_config.ParseBool("autostart", false);

  auto& statistics_storage =
      component_context.FindComponent<components::StatisticsStorage>();
  statistics_holder_ = statistics_storage.GetStorage().RegisterExtender(
      "distlock." + component_config.Name(),
      [this](const ::utils::statistics::StatisticsRequest&) {
        return ExtendStatistics();
      });
}

DistLockComponentBase::~DistLockComponentBase() {
  statistics_holder_.Unregister();
}

DistLockedTask& DistLockComponentBase::GetTask() { return *task_; }

void DistLockComponentBase::StartDistLock() {
  if (autostart_) task_->Start();
}

void DistLockComponentBase::StopDistLock() { task_->Stop(); }

formats::json::Value DistLockComponentBase::ExtendStatistics() {
  auto& stats = task_->GetStatistics();
  bool running = task_->IsRunning();
  auto locked_duration = task_->GetLockedDuration();

  formats::json::ValueBuilder result;
  result["running"] = running ? 1 : 0;
  result["locked"] = locked_duration ? 1 : 0;
  result["locked-for-ms"] =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          locked_duration.value_or(std::chrono::seconds(0)))
          .count();

  result["successes"] = stats.successes.load();
  result["failures"] = stats.failures.load();
  result["watchdog-triggers"] = stats.watchdog_triggers.load();
  result["brain-splits"] = stats.brain_splits.load();
  ::utils::statistics::SolomonLabelValue(result, "distlock_name");

  return result.ExtractValue();
}

}  // namespace postgres
}  // namespace storages
