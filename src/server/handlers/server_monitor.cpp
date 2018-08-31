#include <server/handlers/server_monitor.hpp>

#include <list>

#include <json/writer.h>
#include <boost/algorithm/string.hpp>

#include <components/monitorable_component_base.hpp>

namespace {
const std::string kEngineMonitorDataName = "engine";

void SetSubField(Json::Value& object, std::list<std::string> fields,
                 Json::Value value) {
  if (fields.empty()) {
    object = std::move(value);
  } else {
    const auto field = std::move(fields.front());
    fields.pop_front();
    SetSubField(object[field], std::move(fields), std::move(value));
  }
}

void SetSubField(Json::Value& object, const std::string& path,
                 Json::Value value) {
  std::list<std::string> fields;
  boost::split(fields, path, boost::is_any_of("."));
  SetSubField(object, std::move(fields), std::move(value));
}
}  // namespace

namespace server {
namespace handlers {

ServerMonitor::ServerMonitor(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context, /*is_monitor = */ true),
      components_manager_(component_context.GetManager()) {}

const std::string& ServerMonitor::HandlerName() const {
  static const std::string kHandlerName = kName;
  return kHandlerName;
}

Json::Value ServerMonitor::GetEngineStats(
    components::MonitorVerbosity verbosity) const {
  Json::Value engine_data(Json::objectValue);

  const auto& config = components_manager_.GetConfig();

  if (verbosity == components::MonitorVerbosity::kFull) {
    Json::Value json_task_processors(Json::arrayValue);
    for (const auto& task_processor : config.task_processors) {
      Json::Value json_task_processor(Json::objectValue);
      json_task_processor["name"] = task_processor.name;
      json_task_processor["worker-threads"] =
          Json::UInt64{task_processor.worker_threads};
      json_task_processors[json_task_processors.size()] =
          std::move(json_task_processor);
    }
    engine_data["task-processors"] = std::move(json_task_processors);
  }

  auto coro_stats = components_manager_.GetCoroPool().GetStats();
  {
    Json::Value json_coro_pool(Json::objectValue);

    Json::Value json_coro_stats(Json::objectValue);
    json_coro_stats["active"] = Json::UInt64{coro_stats.active_coroutines};
    json_coro_stats["total"] = Json::UInt64{coro_stats.total_coroutines};
    json_coro_pool["coroutines"] = std::move(json_coro_stats);

    engine_data["coro-pool"] = std::move(json_coro_pool);
  }

  return engine_data;
}

std::string ServerMonitor::HandleRequestThrow(const http::HttpRequest& request,
                                              request::RequestContext&) const {
  using Verbosity = components::MonitorVerbosity;
  const auto verbosity =
      request.GetArg("full") == "1" ? Verbosity::kFull : Verbosity::kTerse;

  Json::Value monitor_data;

  for (const auto& it : components_manager_.GetMonitorableComponentSet()) {
    const auto& component = it.second;

    SetSubField(monitor_data, component->GetMetricsPath(),
                component->GetMonitorData(verbosity));
  }

  monitor_data[kEngineMonitorDataName] = GetEngineStats(verbosity);
  return Json::FastWriter().write(monitor_data);
}  // namespace server

}  // namespace handlers
}  // namespace server
