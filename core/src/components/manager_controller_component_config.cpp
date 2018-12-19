#include <components/manager_controller_component_config.hpp>

namespace {

engine::TaskProcessorSettings ParseTaskProcessorSettings(
    const formats::json::Value& doc) {
  engine::TaskProcessorSettings settings;

  const auto overload_doc = doc["wait_queue_overload"];

  settings.wait_queue_time_limit =
      std::chrono::microseconds(overload_doc["time_limit_us"].asInt64());
  settings.wait_queue_length_limit = overload_doc["length_limit"].asInt64();
  const auto action = overload_doc["action"].asString();

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
    const taxi_config::DocsMap& docs_map)
    : doc(docs_map.Get("USERVER_TASK_PROCESSOR_QOS")) {
  const auto& default_service = doc["default-service"];

  for (auto it = default_service.begin(); it != default_service.end(); ++it)
    settings[it.GetName()] = ParseTaskProcessorSettings(*it);
  default_settings = settings.at("default-task-processor");
}

}  // namespace components
