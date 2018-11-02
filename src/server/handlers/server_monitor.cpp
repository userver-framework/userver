#include <server/handlers/server_monitor.hpp>

#include <list>

#include <boost/algorithm/string.hpp>

#include <components/component_base.hpp>
#include <components/manager_config.hpp>
#include <engine/task/task_processor.hpp>
#include <engine/task/task_processor_pools.hpp>
#include <formats/json/serialize.hpp>
#include <formats/json/value_builder.hpp>

namespace {
const std::string kEngineMonitorDataName = "engine";
}  // namespace

namespace server {
namespace handlers {

ServerMonitor::ServerMonitor(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context, /*is_monitor = */ true),
      components_manager_(component_context.GetManager()),
      statistics_storage_(
          component_context.FindComponent<components::StatisticsStorage>()),
      task_processor_map_(component_context.GetTaskProcessorsMap()) {}

const std::string& ServerMonitor::HandlerName() const {
  static const std::string kHandlerName = kName;
  return kHandlerName;
}

formats::json::ValueBuilder GetTaskProcessorStats(
    const engine::TaskProcessor& task_processor) {
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

  formats::json::ValueBuilder json_context_switch(formats::json::Type::kObject);
  json_context_switch["slow"] = counter.GetTaskSwitchSlow();
  json_context_switch["fast"] = counter.GetTaskSwitchFast();
  json_context_switch["spurious_wakeups"] = counter.GetSpuriousWakeups();
  json_task_processor["context_switch"] = std::move(json_context_switch);
  json_task_processor["worker-threads"] = task_processor.GetWorkerCount();

  return json_task_processor;
}

formats::json::Value ServerMonitor::GetEngineStats(
    utils::statistics::StatisticsRequest) const {
  formats::json::ValueBuilder engine_data(formats::json::Type::kObject);

  formats::json::ValueBuilder json_task_processors(
      formats::json::Type::kObject);
  for (const auto& tp_it : task_processor_map_) {
    const auto& name = tp_it.first;
    const auto& task_processor = tp_it.second;
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
}

std::string ServerMonitor::HandleRequestThrow(const http::HttpRequest& request,
                                              request::RequestContext&) const {
  const auto prefix = request.GetArg("prefix");

  auto statistics_request = utils::statistics::StatisticsRequest();
  formats::json::ValueBuilder monitor_data =
      statistics_storage_.GetStorage().GetAsJson(prefix, statistics_request);

  monitor_data[kEngineMonitorDataName] = GetEngineStats(statistics_request);
  return formats::json::ToString(monitor_data.ExtractValue());
}  // namespace server

}  // namespace handlers
}  // namespace server
