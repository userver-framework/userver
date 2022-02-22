#include <components/manager_controller_component_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

engine::TaskProcessorSettings ParseTaskProcessorSettings(
    const formats::json::Value& doc) {
  engine::TaskProcessorSettings settings;

  const auto overload_doc = doc["wait_queue_overload"];

  settings.wait_queue_time_limit =
      std::chrono::microseconds(overload_doc["time_limit_us"].As<int64_t>());
  settings.wait_queue_length_limit = overload_doc["length_limit"].As<int64_t>();
  settings.sensor_wait_queue_time_limit = std::chrono::microseconds(
      overload_doc["sensor_time_limit_us"].As<int64_t>(3000));
  const auto action = overload_doc["action"].As<std::string>();

  using oa = engine::TaskProcessorSettings::OverloadAction;
  if (action == "cancel")
    settings.overload_action = oa::kCancel;
  else if (action == "ignore")
    settings.overload_action = oa::kIgnore;
  else {
    LOG_ERROR() << "Unknown wait_queue_overload_action: " << action;
  }

  return settings;
}

}  // namespace

namespace components {

// TODO: parse both "default-service" and current service

ManagerControllerTaxiConfig::ManagerControllerTaxiConfig(
    const dynamic_config::DocsMap& docs_map)
    : tp_doc(docs_map.Get("USERVER_TASK_PROCESSOR_QOS")) {
  const auto& default_service = tp_doc["default-service"];

  for (const auto& [name, value] : Items(default_service)) {
    settings[name] = ParseTaskProcessorSettings(value);
  }
  default_settings = settings.at("default-task-processor");

  auto profiler_doc = docs_map.Get("USERVER_TASK_PROCESSOR_PROFILER_DEBUG");
  for (const auto& [name, value] : Items(profiler_doc)) {
    auto profiler_enabled = value["enabled"].As<bool>();
    if (profiler_enabled) {
      // If the key is missing, make a copy of default settings and fill the
      // profiler part.
      auto it = settings.emplace(name, default_settings).first;
      auto& tp_settings = it->second;

      tp_settings.profiler_execution_slice_threshold =
          std::chrono::microseconds(
              value["execution-slice-threshold-us"].As<int>());
      tp_settings.profiler_force_stacktrace =
          value["profiler-force-stacktrace"].As<bool>(false);
    }
  }
}

}  // namespace components

USERVER_NAMESPACE_END
