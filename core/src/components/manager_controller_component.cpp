#include <userver/components/manager_controller_component.hpp>

#include <components/manager_config.hpp>
#include <components/manager_controller_component_config.hpp>
#include <engine/task/task_processor.hpp>
#include <engine/task/task_processor_pools.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/logging/component.hpp>
#include <userver/utils/statistics/metadata.hpp>

#include <components/manager.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

void DumpMetric(utils::statistics::Writer& writer,
                const engine::TaskProcessor& task_processor) {
  const auto& counter = task_processor.GetTaskCounter();

  const auto destroyed = counter.GetDestroyedTasks();
  const auto created = counter.GetCreatedTasks();
  const auto stopped = counter.GetStoppedTasks();
  const auto started = counter.GetStartedTasks();

  // TODO report RATE metrics
  if (auto tasks = writer["tasks"]) {
    tasks["created"] = created.value;
    tasks["alive"] = created.value - std::min(destroyed, created).value;
    tasks["running"] = started.value - std::min(stopped, started).value;
    tasks["queued"] = task_processor.GetTaskQueueSize();
    tasks["finished"] = stopped.value;
    tasks["cancelled"] = counter.GetCancelledTasks().value;
  }

  writer["errors"].ValueWithLabels(
      counter.GetTasksOverload().value,
      {{"task_processor_error", "wait_queue_overload"}});

  if (auto context_switch = writer["context_switch"]) {
    context_switch["slow"] = counter.GetTaskSwitchSlow().value;
    context_switch["fast"] = counter.GetTaskSwitchFast().value;
    context_switch["spurious_wakeups"] = counter.GetSpuriousWakeups().value;

    context_switch["overloaded"] = counter.GetTasksOverloadSensor().value;
    context_switch["no_overloaded"] = counter.GetTasksNoOverloadSensor().value;
  }

  writer["worker-threads"] = task_processor.GetWorkerCount();
}

}  // namespace engine

namespace components {

ManagerControllerComponent::ManagerControllerComponent(
    const components::ComponentConfig&,
    const components::ComponentContext& context)
    : components_manager_(context.GetManager()) {
  auto& storage =
      context.FindComponent<components::StatisticsStorage>().GetStorage();

  auto config_source = context.FindComponent<DynamicConfig>().GetSource();
  config_subscription_ = config_source.UpdateAndListen(
      this, "engine_controller", &ManagerControllerComponent::OnConfigUpdate);

  statistics_holder_ = storage.RegisterWriter(
      "engine", [this](utils::statistics::Writer& writer) {
        return WriteStatistics(writer);
      });

  auto& logger_component = context.FindComponent<components::Logging>();
  for (const auto& [name, task_processor] :
       components_manager_.GetTaskProcessorsMap()) {
    const auto& logger_name = task_processor->GetTaskTraceLoggerName();
    if (!logger_name.empty()) {
      auto logger = logger_component.GetLogger(logger_name);
      task_processor->SetTaskTraceLogger(std::move(logger));
    }
  }
}

ManagerControllerComponent::~ManagerControllerComponent() {
  statistics_holder_.Unregister();
  config_subscription_.Unsubscribe();
}

void ManagerControllerComponent::WriteStatistics(
    utils::statistics::Writer& writer) {
  // task processors
  for (const auto& [name, task_processor] :
       components_manager_.GetTaskProcessorsMap()) {
    writer["task-processors"].ValueWithLabels(*task_processor,
                                              {{"task_processor", name}});
  }

  // ev-threads
  const auto& pools_ptr = components_manager_.GetTaskProcessorPools();
  auto& ev_thread_pool = pools_ptr->EventThreadPool();
  for (auto* thread : ev_thread_pool.NextThreads(ev_thread_pool.GetSize())) {
    writer["ev-threads"]["cpu-load-percent"].ValueWithLabels(
        thread->GetCurrentLoadPercent(),
        {{"ev_thread_name", thread->GetName()}});
  }

  // coroutines
  if (auto coro_pool = writer["coro-pool"]) {
    if (auto coro_stats = coro_pool["coroutines"]) {
      auto stats =
          components_manager_.GetTaskProcessorPools()->GetCoroPool().GetStats();
      coro_stats["active"] = stats.active_coroutines;
      coro_stats["total"] = stats.total_coroutines;
    }
  }

  // misc
  writer["uptime-seconds"] =
      std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::steady_clock::now() - components_manager_.GetStartTime())
          .count();
  writer["load-ms"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                          components_manager_.GetLoadDuration())
                          .count();
}

void ManagerControllerComponent::OnConfigUpdate(
    const dynamic_config::Snapshot& cfg) {
  auto config = cfg.Get<ManagerControllerDynamicConfig>();

  for (const auto& [name, task_processor] :
       components_manager_.GetTaskProcessorsMap()) {
    auto it = config.settings.find(name);
    if (it != config.settings.end()) {
      task_processor->SetSettings(it->second);
    } else {
      task_processor->SetSettings(config.default_settings);
    }
  }
}

}  // namespace components

USERVER_NAMESPACE_END
