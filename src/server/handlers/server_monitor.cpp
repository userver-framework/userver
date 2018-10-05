#include <server/handlers/server_monitor.hpp>

#include <list>

#include <boost/algorithm/string.hpp>

#include <components/component_base.hpp>
#include <components/manager_config.hpp>
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
          component_context
              .FindComponentRequired<components::StatisticsStorage>()) {}

const std::string& ServerMonitor::HandlerName() const {
  static const std::string kHandlerName = kName;
  return kHandlerName;
}

formats::json::Value ServerMonitor::GetEngineStats(
    utils::statistics::Verbosity verbosity) const {
  formats::json::ValueBuilder engine_data(formats::json::Type::kObject);

  const auto& config = components_manager_.GetConfig();

  if (verbosity == utils::statistics::Verbosity::kFull) {
    formats::json::ValueBuilder json_task_processors(
        formats::json::Type::kArray);
    for (const auto& task_processor : config.task_processors) {
      formats::json::ValueBuilder json_task_processor(
          formats::json::Type::kObject);
      json_task_processor["name"] = task_processor.name;
      json_task_processor["worker-threads"] = task_processor.worker_threads;
      json_task_processors.PushBack(std::move(json_task_processor));
    }
    engine_data["task-processors"] = std::move(json_task_processors);
  }

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
  using Verbosity = utils::statistics::Verbosity;
  const auto verbosity =
      request.GetArg("full") == "1" ? Verbosity::kFull : Verbosity::kTerse;
  const auto prefix = request.GetArg("prefix");

  formats::json::ValueBuilder monitor_data =
      statistics_storage_->GetStorage().GetAsJson(prefix, verbosity);

  monitor_data[kEngineMonitorDataName] = GetEngineStats(verbosity);
  return formats::json::ToString(monitor_data.ExtractValue());
}  // namespace server

}  // namespace handlers
}  // namespace server
