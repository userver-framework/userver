#include <components/manager_controller_component_config.hpp>

#include <userver/dynamic_config/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {
namespace {

constexpr dynamic_config::DefaultAsJsonString kQosDefault{R"(
{
  "default-service": {
    "default-task-processor": {
      "wait_queue_overload": {
        "action": "ignore",
        "length_limit": 5000,
        "time_limit_us": 3000
      }
    }
  }
}
)"};

constexpr dynamic_config::DefaultAsJsonString kProfilerDefault{R"(
{
  "fs-task-processor": {
    "enabled": false,
    "execution-slice-threshold-us": 1000000
  },
  "main-task-processor": {
    "enabled": false,
    "execution-slice-threshold-us": 2000
  }
}
)"};

}  // namespace

ManagerControllerDynamicConfig ManagerControllerDynamicConfig::Parse(
    const dynamic_config::DocsMap& docs_map) {
  const auto tp_doc = docs_map.Get("USERVER_TASK_PROCESSOR_QOS");
  const auto& default_service = tp_doc["default-service"];

  ManagerControllerDynamicConfig result;
  result.settings =
      default_service
          .As<std::unordered_map<std::string, engine::TaskProcessorSettings>>();
  result.default_settings = result.settings.at("default-task-processor");

  const auto profiler_doc =
      docs_map.Get("USERVER_TASK_PROCESSOR_PROFILER_DEBUG");
  for (const auto& [name, value] : Items(profiler_doc)) {
    auto profiler_enabled = value["enabled"].As<bool>();
    if (profiler_enabled) {
      // If the key is missing, make a copy of default settings and fill the
      // profiler part.
      auto it = result.settings.emplace(name, result.default_settings).first;
      auto& tp_settings = it->second;

      tp_settings.profiler_execution_slice_threshold =
          std::chrono::microseconds{
              value["execution-slice-threshold-us"].As<int>()};
      tp_settings.profiler_force_stacktrace =
          value["profiler-force-stacktrace"].As<bool>(false);
    }
  }

  return result;
}

const dynamic_config::Key<ManagerControllerDynamicConfig>
    kManagerControllerDynamicConfig{
        ManagerControllerDynamicConfig::Parse,
        {
            {"USERVER_TASK_PROCESSOR_QOS", kQosDefault},
            {"USERVER_TASK_PROCESSOR_PROFILER_DEBUG", kProfilerDefault},
        },
    };

}  // namespace components

USERVER_NAMESPACE_END
