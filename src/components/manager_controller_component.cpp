#include <components/manager_controller_component.hpp>

#include <components/manager_config.hpp>
#include <components/manager_controller_component_config.hpp>
#include <components/statistics_storage.hpp>
#include <engine/task/task_processor.hpp>
#include <engine/task/task_processor_pools.hpp>
#include <formats/json/serialize.hpp>
#include <formats/json/value_builder.hpp>
#include <utils/statistics/aggregate_values_format_json.hpp>

#include <taxi_config/value.hpp>

namespace {
const std::string kEngineMonitorDataName = "engine";

}  // namespace

namespace components {

ManagerControllerComponent::ManagerControllerComponent(
    const components::ComponentConfig&,
    const components::ComponentContext& context)
    : components_manager_(context.GetManager()),
      task_processor_map_(context.GetTaskProcessorsMap()) {
  auto& storage =
      context.FindComponent<components::StatisticsStorage>().GetStorage();
  statistics_holder_ = storage.RegisterExtender(
      kEngineMonitorDataName,
      std::bind(&ManagerControllerComponent::ExtendStatistics, this,
                std::placeholders::_1));

  auto& config = context.FindComponent<TaxiConfig>();
  config_subscription_ =
      config.AddListener(this, &ManagerControllerComponent::OnConfigUpdate);
  OnConfigUpdate(config.Get());
}

ManagerControllerComponent::~ManagerControllerComponent() {
  statistics_holder_.Unregister();
  config_subscription_.Unsubscribe();
}

formats::json::ValueBuilder ManagerControllerComponent::GetTaskProcessorStats(
    const engine::TaskProcessor& task_processor) const {
  const auto& counter = task_processor.GetTaskCounter();

  const auto current = counter.GetCurrentValue();
  const auto created = counter.GetCreatedTasks();
  const auto cancelled = counter.GetCancelledTasks();
  const auto queued = task_processor.GetTaskQueueSize();

  formats::json::ValueBuilder json_task_processor(formats::json::Type::kObject);

  formats::json::ValueBuilder json_tasks(formats::json::Type::kObject);
  json_tasks["created"] = created;
  json_tasks["alive"] = current;
  json_tasks["queued"] = queued;
  json_tasks["finished"] = created - current;
  json_tasks["cancelled"] = cancelled;
  json_task_processor["tasks"] = std::move(json_tasks);

  formats::json::ValueBuilder json_errors(formats::json::Type::kObject);
  json_errors["wait_queue_overload"] = counter.GetTasksOverload();
  json_task_processor["errors"] = std::move(json_errors);

  formats::json::ValueBuilder json_context_switch(formats::json::Type::kObject);
  json_context_switch["slow"] = counter.GetTaskSwitchSlow();
  json_context_switch["fast"] = counter.GetTaskSwitchFast();
  json_context_switch["spurious_wakeups"] = counter.GetSpuriousWakeups();
  json_task_processor["context_switch"] = std::move(json_context_switch);

  json_task_processor["worker-threads"] = task_processor.GetWorkerCount();

#ifdef USERVER_PROFILER
  json_task_processor["profiler"] = utils::statistics::AggregatedValuesToJson(
      counter.GetTaskExecutionTimings(), "-us");
#endif  // USERVER_PROFILER

  return json_task_processor;
}

formats::json::Value ManagerControllerComponent::ExtendStatistics(
    const utils::statistics::StatisticsRequest& /*request*/) {
  formats::json::ValueBuilder engine_data(formats::json::Type::kObject);

  formats::json::ValueBuilder json_task_processors(
      formats::json::Type::kObject);
  for (const auto& tp_item : task_processor_map_) {
    const auto& name = tp_item.first;
    const auto& task_processor = tp_item.second;
    json_task_processors[name] = GetTaskProcessorStats(*task_processor);
  }
  engine_data["task-processors"]["by-name"] = std::move(json_task_processors);

  auto coro_stats =
      components_manager_.GetTaskProcessorPools()->GetCoroPool().GetStats();
  {
    formats::json::ValueBuilder json_coro_pool(formats::json::Type::kObject);

    formats::json::ValueBuilder json_coro_stats(formats::json::Type::kObject);
    json_coro_stats["active"] = coro_stats.active_coroutines;
    json_coro_stats["total"] = coro_stats.total_coroutines;
    json_coro_pool["coroutines"] = std::move(json_coro_stats);

    engine_data["coro-pool"] = std::move(json_coro_pool);
  }

  return engine_data.ExtractValue();
}  // namespace components

void ManagerControllerComponent::OnConfigUpdate(const TaxiConfigPtr& cfg) {
  auto config = cfg->Get<ManagerControllerTaxiConfig>();

  for (const auto& it : task_processor_map_) {
    it.second->SetSettings(config.default_settings);
  }
}

}  // namespace components
