#include <components/manager_controller_component_config.hpp>

#include <fmt/format.h>

#include <userver/dynamic_config/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

ManagerControllerDynamicConfig::ManagerControllerDynamicConfig(
    const dynamic_config::DocsMap& docs_map)
    : tp_doc(docs_map.Get("USERVER_TASK_PROCESSOR_QOS")) {
  const auto& default_service = tp_doc["default-service"];

  settings =
      default_service
          .As<std::unordered_map<std::string, engine::TaskProcessorSettings>>();
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
