#include <server/handlers/server_monitor.hpp>

#include <list>

#include <boost/algorithm/string.hpp>
#include <formats/json/serialize.hpp>
#include <formats/json/value_builder.hpp>

#include <components/monitorable_component_base.hpp>

namespace {
const std::string kEngineMonitorDataName = "engine";

void SetSubField(formats::json::ValueBuilder& object,
                 std::list<std::string> fields, formats::json::Value value) {
  if (fields.empty()) {
    object = std::move(value);
  } else {
    const auto field = std::move(fields.front());
    fields.pop_front();
    auto subobj = object[field];
    SetSubField(subobj, std::move(fields), std::move(value));
  }
}

void SetSubField(formats::json::ValueBuilder& object, const std::string& path,
                 formats::json::Value value) {
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

formats::json::Value ServerMonitor::GetEngineStats(
    components::MonitorVerbosity verbosity) const {
  formats::json::ValueBuilder engine_data(formats::json::Type::kObject);

  const auto& config = components_manager_.GetConfig();

  if (verbosity == components::MonitorVerbosity::kFull) {
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

  auto coro_stats = components_manager_.GetCoroPool().GetStats();
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
  using Verbosity = components::MonitorVerbosity;
  const auto verbosity =
      request.GetArg("full") == "1" ? Verbosity::kFull : Verbosity::kTerse;

  formats::json::ValueBuilder monitor_data;

  for (const auto& it : components_manager_.GetMonitorableComponentSet()) {
    const auto& component = it.second;

    SetSubField(monitor_data, component->GetMetricsPath(),
                component->GetMonitorData(verbosity));
  }

  monitor_data[kEngineMonitorDataName] = GetEngineStats(verbosity);
  return formats::json::ToString(monitor_data.ExtractValue());
}  // namespace server

}  // namespace handlers
}  // namespace server
